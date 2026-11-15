/* X3 independent views over the native world streams, resource controllers
 * and completed Cx4 draw submissions.
 * Rendering never selects a player or modifies guest memory. */
#include "x3_display.h"
#include "x3_fixed_arena.h"
#include "x3_coop_runtime.h"
#include "x3_coop_present.h"
#include "x3_coop_render.h"
#include "x3_coop_view.h"
#include "x3_coop_hud.h"
#include "x3_coop_menu.h"
#include "x3_native_resources.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
extern Snes *g_snes;
extern uint8_t g_ram[0x20000];
enum { WORLD_PIECES=112, EXTRA_PIECES=256, RESOURCE_PHASES=32 };
typedef struct ResourceEvent {uint16_t x,y;uint8_t kind,phase;} ResourceEvent;
typedef struct Palette {uint16_t colors[256];uint8_t valid[256];} Palette;
typedef struct IndependentFrame {
  const uint8_t *rom;size_t size;
  uint8_t ram[0x20000];
  int x,y,dx,dy;
  unsigned stage,events,count;
  bool closed_arena;
  ResourceEvent event[256];
  uint8_t phase[3][8192];
  PpuExtraObject objects[EXTRA_PIECES];
  uint16_t pixels[WORLD_PIECES][256];
  uint16_t boss_hud[7][256];
  uint16_t hud[2][X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT];
} IndependentFrame;
static IndependentFrame independent_frames[2];
static IndependentFrame *active_frame=&independent_frames[0];
#define frame (*active_frame)
static uint32_t coop_pixels[2*kPpuMaxFrameWidth*224];
static unsigned panel_width=256;
static unsigned composed_width;
static bool composed_split;
void X3Display_InvalidateCoopFrame(void){composed_width=0;}
void X3Display_SetCoopPanelWidth(unsigned width){
  unsigned next=width>=256 && width<=kPpuMaxFrameWidth && !(width&1)?width:256;
  if(next!=panel_width)composed_width=0;
  panel_width=next;
}
static bool rendered_private;
bool X3Display_RenderedPrivateFrame(void){return rendered_private;}
/* Derived ROM caches have finite capacity and are never simulation state. */
static unsigned cache_stage=UINT_MAX;
static const uint8_t *cache_rom;
static uint8_t chr[16][65536],chr_valid[16][2048],chr_ready[16];
static Palette palettes[RESOURCE_PHASES];
static uint8_t palette_ready[RESOURCE_PHASES];
/* Native decompressed files are immutable, bounded ROM-derived caches. */
static uint8_t files[X3_RESOURCE_KEYS][X3_RESOURCE_FILE_MAX],file_ready[X3_RESOURCE_KEYS];
static size_t file_length[X3_RESOURCE_KEYS];

static unsigned word(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static bool rom_read(unsigned a,unsigned n,unsigned *v){
  size_t p=((a>>16)&127)*32768+(a&32767);
  if((a&65535)<32768 || p>frame.size || n>frame.size-p)return false;
  *v=0;for(unsigned i=0;i<n;++i)*v|=(unsigned)frame.rom[p+i]<<(i*8);return true;
}
/* Native06:A085/A172 directories contain15 stage lists; stage14 aliases11.
 * The next distinct list, rather than the next stage, bounds normal phases. */
static unsigned resource_list(unsigned directory,unsigned phase){
  unsigned lists[18],data=65535,start,end,list,stages=directory==0x088623?18:15;
  for(unsigned i=0;i<stages;++i){
    if(!rom_read(directory+i*2,2,&lists[i]) || lists[i]<30 || lists[i]>1024)return 0;
    unsigned first;if(!rom_read(directory+lists[i],2,&first) || first<30)return 0;
    if(first<data)data=first;
  }
  if(frame.stage>=15)return 0;
  start=lists[frame.stage];end=data;
  for(unsigned i=0;i<stages;++i)if(lists[i]>start && lists[i]<end)end=lists[i];
  if(end<start || ((end-start)&1))return 0;
  unsigned phases=(end-start)/2;
  if(directory==0x06a172 && (frame.stage==4 || frame.stage==6))phases+=5;
  if(phase>=phases || !rom_read(directory+start+phase*2,2,&list))return 0;
  return directory+list;
}
static void prepare_chr(unsigned phase){
  if(phase>=16 || chr_ready[phase])return;
  chr_ready[phase]=1;
  unsigned a=resource_list(0x06a085,phase);
  for(unsigned n=0;a && n<64;++n,a+=9){
    unsigned count,dest,source;
    if(!rom_read(a,2,&count) || !count)return;
    if(!rom_read(a+2,2,&dest) || !rom_read(a+4,3,&source) ||
       (dest&15) || (count&31) || dest*2+count>65536)return;
    for(unsigned i=0;i<count;++i){unsigned b;
      if(!rom_read((source&0xff0000)|((source+i)&65535),1,&b))return;
      chr[phase][dest*2+i]=(uint8_t)b;
    }
    memset(chr_valid[phase]+dest/16,1,count/32);
  }
}
static const Palette *prepare_palette(unsigned phase){
  if(phase>=RESOURCE_PHASES)return NULL;
  if(palette_ready[phase])return &palettes[phase];
  palette_ready[phase]=1;
  Palette *p=&palettes[phase];
  /* Retail uploads are partial. Resolve retained groups from ROM rather
   * than borrowing another view's current CGRAM. */
  if(phase)*p=*prepare_palette(phase-1);
  unsigned a=resource_list(0x06a172,phase);
  for(unsigned n=0;a && n<64;++n,a+=3){
    unsigned source,dest;
    if(!rom_read(a,2,&source) || source==65535)return p;
    if(!rom_read(a+2,1,&dest) || dest>240)return p;
    for(unsigned i=0;i<16;++i){unsigned c;
      if(!rom_read(0x0c0000|((source+i*2)&65535),2,&c))return p;
      p->colors[dest+i]=(uint16_t)(c&32767);p->valid[dest+i]=1;
    }
  }
  return p;
}
static void prepare_events(void){
  frame.events=0;
  bool first_phase[3]={true,true,true};
  memset(frame.phase[0],frame.ram[0x1f19],8192);
  memset(frame.phase[1],frame.ram[0x1f1a],8192);
  memset(frame.phase[2],frame.ram[0x1f18],8192);
  for(unsigned col=0;col<256;++col){
    unsigned first=word(frame.ram+0xf800+col*2),end=word(frame.ram+0xf802+col*2);
    if(first<0xfa02 || end>0xfffc || end<first || (end-first)%5)continue;
    for(unsigned flag=first;flag<end;flag+=5){
      unsigned a=word(frame.ram+flag+3),kind,id,param,y,x;
      if(a<32768 || !rom_read(0x3c0000|a,1,&kind) || (kind&15)!=2 ||
         !rom_read(0x3c0000|((a+3)&65535),1,&id) ||
         (id<0x15 || id>0x1a) ||
         !rom_read(0x3c0000|((a+4)&65535),1,&param) ||
         !rom_read(0x3c0000|((a+1)&65535),2,&y) ||
         !rom_read(0x3c0000|((a+5)&65535),2,&x))continue;
      x&=8191;y&=8191;
      if(id<=0x17){unsigned kind=id==0x15?2:id-0x16;
        if(first_phase[kind]){memset(frame.phase[kind],param>>4,8192);first_phase[kind]=false;}
        memset(frame.phase[kind]+x,param&15,8192-x);
      }else if(frame.events<256)
        frame.event[frame.events++]=(ResourceEvent){(uint16_t)x,(uint16_t)y,(uint8_t)id,(uint8_t)param};
    }
  }
}
static unsigned resource_phase(unsigned kind,int x,int y){
  unsigned phase=x>=0 && x<8192?frame.phase[kind][x]:frame.ram[kind==2?0x1f18:kind?0x1f1a:0x1f19];
  /* Native39:BF8D uses high/low nibbles on the two sides of each
   * controller. Loaded records retain their authored column ordering. */
  for(unsigned i=0;i<frame.events;++i){const ResourceEvent *e=&frame.event[i];
    if(e->kind==(kind==2?0x18:0x19+kind) && x>=(int)e->x-128 && x<(int)e->x+128)
      phase=y<e->y?e->phase>>4:e->phase&15;
  }
  if(kind==1 && ((frame.stage==4 && (frame.ram[0x1fc9]&64)) ||
             (frame.stage==6 && (frame.ram[0x1fc3]&64))))phase+=5;
  return phase;
}
static unsigned tile_pixel(const uint16_t *v,unsigned a,unsigned x,unsigned y,unsigned bpp){
  unsigned value=0;
  for(unsigned pair=0;pair<bpp/2;++pair){unsigned bits=v[(a+pair*8+y)&32767];
    value|=((bits>>(7-x))&1)<<(pair*2);value|=((bits>>(15-x))&1)<<(pair*2+1);
  }
  return value;
}
typedef struct ObjectResource {
  const uint8_t *tiles;
  X3ResourceSegment segments[X3_RESOURCE_SEGMENTS];
  unsigned count,tile,attributes;
} ObjectResource;
static bool object_resource(const X3CoopWorldDraw *draw,uint16_t colors[256],ObjectResource *out){
  const uint8_t *b=draw->body;
  bool armor=false;
  if(draw->address==0xcc8 && !b[10])for(unsigned variant=0;variant<4;++variant){
    unsigned group;if(rom_read(0x06b517+variant,1,&group) && group==b[0x16])armor=true;
  }
  if(draw->owner || (!b[10] && !armor))return false;
  /* The common native02:E15C constructor selects group/file from06:E28E.
   * A customized group is left on its original native path. */
  unsigned group,key;
  if(armor){
    /* Playable armor03:9428 uses file25's placement cache, independently of
     * the canonical body's room. Its native friendly palette is fixed by
     * 03:9479; it does not use the enemy constructor's palette offset. */
    group=b[0x16];key=0x25;
  }else if(draw->address>=0xd18 && draw->address<0x10d8){
    if(!rom_read(0x06e28e + (b[10]-1)*5,1,&group) ||
       !rom_read(0x06e28f+(b[10]-1)*5,1,&key))return false;
  }else if(draw->address>=0x1518 && draw->address<0x1818 && !((draw->address-0x1518)%48)){
    /* Native01:E069/E073 selects these door files/groups. A remote gate
     * can initialize before its region uploads the native placement cache. */
    if(b[10]==7 || b[10]==0x10 || b[10]==0x11){group=b[10]==7?0x1b:0x4c;key=b[10]==7?0x19:0x56;}
    else if(b[10]==11){group=0x38;key=0x36;} /* Native01:E848 heart. */
    else if(b[10]==5){group=0x96;key=0x8c;} /* Native01:DAE1 subtank. */
    else return false;
  }else return false;
  if(group!=b[0x16] || key>=X3_RESOURCE_KEYS)return false;
  unsigned phase=resource_phase(2,(int)word(b+5),(int)word(b+8));
  unsigned a=resource_list(0x088623,phase),file=255;
  for(unsigned i=0;a && i<64;++i,a+=6){
    if(!rom_read(a,1,&file) || file==255)return false;
    if(file==key)break;
  }
  if(!a || file!=key)return false;
  unsigned target,palette,offset;
  if(!rom_read(a+1,2,&target) || target>=32768 || !rom_read(a+3,2,&palette) || !rom_read(a+5,1,&offset))return false;
  out->tile=(target>>4)&255;out->attributes=(target>>12)|(offset>>3)|0x20;
  if(armor)out->attributes=(target>>12)|(b[0x11]&62);
  if(draw->address>=0xd18 && draw->address<0x10d8 && (b[10]==0x4d || b[10]==0x5c)){
    /* Capsule13:C075 and hologram05:C863 use A0/80, forty tiles before
     * their file88/89 uploads at E0/C0. Their layouts include that40 bias;
     * applying the common constructor's tile base makes every piece miss
     * the file's upload segments. Keep their native priority/palette too. */
    out->tile=(out->tile-0x40)&255;out->attributes=(target>>12)|(b[0x11]&62);
  }
  /* Unlike ordinary resource actors, native01:DB03 keeps the common pickup
   * palette and the body's priority, borrowing only file8C's tile bank. */
  if(draw->address>=0x1518 && b[10]==5)out->attributes=(target>>12)|(b[0x11]&62);
  /* Preserve native animation, flashing and uploaded art when the completed
   * local cache already has this file's actual placement and palette. */
  bool hologram=draw->address>=0xd18 && draw->address<0x10d8 && b[10]==0x5c;
  if(!armor && !hologram && phase==frame.ram[0x1f18] && b[0x18]==out->tile && (b[0x11]&63)==out->attributes)return false;
  if(!file_ready[key]){
    file_ready[key]=X3ResourceDecode(frame.rom,frame.size,key,files[key],sizeof(files[key]),&file_length[key])?1:2;
  }
  if(file_ready[key]!=1 || !X3ResourceLayout(frame.rom,frame.size,key,target,out->segments,&out->count) ||
     (!armor && !X3ResourcePalette(frame.rom,frame.size,palette,offset,colors)))return false;
  /* Native05:C846 loads the hologram's fixed E4 palette into OBJ palette0.
   * It must not borrow a surviving Zero's current body colors. */
  if(hologram && !X3ResourcePalette(frame.rom,frame.size,0xe4,0,colors))return false;
  /* Native DMA rounds some uploads beyond the meaningful decoded file
   * (door56:352 bytes in a384-byte upload). Keep real tiles available and
   * leave padding transparent, with every pixel read bounded to file data. */
  for(unsigned i=0;i<out->count;++i){X3ResourceSegment *s=&out->segments[i];
    size_t available=s->source<file_length[key]?file_length[key]-s->source:0;
    if(s->length>available)s->length=(uint16_t)available;
  }
  out->tiles=files[key];return true;
}
static unsigned resource_pixel(const ObjectResource *resource,unsigned address,unsigned x,unsigned y){
  unsigned at=address*2+(y&7)*2;
  for(unsigned i=0;i<resource->count;++i){
    const X3ResourceSegment *s=&resource->segments[i];unsigned base=s->destination*2;
    if(at<base || at+17>=base+s->length)continue;
    const uint8_t *bits=resource->tiles+s->source+at-base;unsigned shift=7-(x&7);
    return ((bits[0]>>shift)&1)|(((bits[1]>>shift)&1)<<1)|(((bits[16]>>shift)&1)<<2)|(((bits[17]>>shift)&1)<<3);
  }
  return 0;
}
static bool stage_tile(unsigned layer,int x,int y,unsigned *tile){
  if(x<0 || y<0 || x>=8192 || y>=8192)return false;
  unsigned layout=layer?0xec00:0xe800,defs=layer?0xa600:0x2800,ptr=layer?0x9c8:0x9c5;
  unsigned sid=frame.ram[layout+(unsigned)(y>>8)*32+(unsigned)(x>>8)];
  unsigned a=(defs+sid*512+((y>>4)&15)*32+((x>>4)&15)*2)&65535;
  unsigned block=frame.ram[a]|(unsigned)frame.ram[(a+1)&65535]<<8;
  a=(word(frame.ram+ptr)+block*8+((y>>3)&1)*4+((x>>3)&1)*2)&65535;
  unsigned lo=cart_read(g_snes->cart,frame.ram[ptr+2],(uint16_t)a);
  unsigned hi=cart_read(g_snes->cart,frame.ram[ptr+2],(uint16_t)(a+1));
  *tile=lo|(hi<<8);return true;
}
/* Kaiser draws its transformed head/torso into the live BG2 object canvas,
 * not the ordinary parallax stage map. The original closed arena is one
 * native field; do not repeat this circular canvas into a distant view or
 * wider margins. Pixel Y follows the PPU's visible scanlines1..224. */
static bool object_canvas(unsigned layer){
  return layer==1 && frame.stage==13 && frame.ram[0x1e99]==14;
}
static bool canvas_contains(int x,int y){return x>=0 && x<256 && y>=1 && y<=224;}
static uint16_t background(const Ppu *p,unsigned layer,int x,int y,int *color){
  static const unsigned low[]={8,7,1},high[]={12,11,3};
  if(frame.closed_arena && layer<2){int native_x=x+frame.dx;
    if(native_x<0 || native_x>=256){if(layer==1)return 0;x=(native_x<0?0:255)-frame.dx;}
  }
  unsigned bpp=layer==2?2:4,size=PPU_bigTiles(p,layer)?16:8;
  if(p->mosaic&(1u<<layer)){
    int m=(p->mosaic>>4)+1;
    int mod=x%m;if(mod<0)mod+=m;x-=mod;
    y-=(y-p->mosaicStartLine)%m;
  }
  bool canvas=object_canvas(layer);
  int source_x=x+(canvas?frame.dx:0),source_y=y+(canvas?frame.dy:0);
  if(canvas && !canvas_contains(source_x,source_y))return 0;
  int px=(source_x+p->hScroll[layer])&1023,py=(source_y+p->vScroll[layer])&1023;
  unsigned sc=p->bgXsc[layer],tx=(unsigned)px/size,ty=(unsigned)py/size;
  unsigned a=(sc&0xfc)*256+(tx&31)+(ty&31)*32;
  if((sc&1) && (tx&32))a+=1024;
  if((sc&2) && (ty&32))a+=(sc&1)?2048:1024;
  unsigned tile=p->vram[a&32767];int asset_x=-1,asset_y=-1;
  unsigned mode=frame.ram[0x1e99];
  bool stream=!canvas && (layer==0 || (layer==1 && (mode==2 || mode==4 || mode==14 || mode==16)));
  if(layer<2 && size==8 && stream){
    int wx,wy;
    if(!layer){
      wx=X3Display_ExpandStageScroll((uint16_t)word(frame.ram+0x1e5d),p->hScroll[0])+frame.dx+x;
      wy=X3Display_ExpandStageScroll((uint16_t)word(frame.ram+0x1e60),p->vScroll[0])+frame.dy+y;
    }else{
      int sx=word(frame.ram+0x1e9d),sy=word(frame.ram+0x1ea0);
      wx=sx+(((p->hScroll[1]-sx+256)&511)-256)+x;
      wy=sy+(((p->vScroll[1]-sy+128)&255)-128)+y;
      int nx=word(frame.ram+0x1e5d),ny=word(frame.ram+0x1e60);
      wx+=(mode==2 || mode==14)?((nx+frame.dx)>>1)-(nx>>1):frame.dx;
      wy+=(mode==2 || mode==16)?((ny+frame.dy)>>1)-(ny>>1):frame.dy;
    }
    /* Match the local compositor using this layer's own source. A slower
     * parallax stream can still cross its finite origin after the foreground
     * has left the edge. Native columns and nonnegative sources stay exact. */
    if(x<0 && wx<0)wx=((wx%32)+32)%32;
    if(!stage_tile(layer,wx,wy,&tile))return 0;
    px=wx;py=wy;
    asset_x=layer && (mode==2 || mode==14)?wx*2-128:wx;
    asset_y=layer && (mode==2 || mode==16)?wy*2-112:wy;
  }else if(layer==2 && (x<0 || x>=256) &&
           !(p->screenEnabled[1]==4 && !(p->screenEnabled[0]&4)))return 0;
  int cx=px&(size-1),cy=py&(size-1);
  if(tile&0x4000)cx=(int)size-1-cx;
  if(tile&0x8000)cy=(int)size-1-cy;
  unsigned number=((tile&1023)+(unsigned)cx/8+(unsigned)cy/8*16)&1023;
  a=(PPU_bgTileAdr(p,layer)+number*bpp*4)&32767;
  unsigned pixel;const uint8_t *bits=NULL;
  if(asset_x>=0 && asset_x<8192 && asset_y>=0 && asset_y<8192){
    unsigned phase=resource_phase(0,asset_x,asset_y);prepare_chr(phase);
    /* A completed native upload supplies animated/live tiles for its own
     * phase. A pending upload must resolve from the destination's source. */
    if(phase<16 && chr_valid[phase][a/16] &&
       (phase!=frame.ram[0x1f19] || frame.ram[0x50]))bits=chr[phase]+a*2;
  }
  if(bits){bits+=(cy&7)*2;unsigned shift=7-(cx&7);
    pixel=((bits[0]>>shift)&1)|(((bits[1]>>shift)&1)<<1)|
          (((bits[16]>>shift)&1)<<2)|(((bits[17]>>shift)&1)<<3);
  }else pixel=tile_pixel(p->vram,a,(unsigned)cx&7,(unsigned)cy&7,bpp);
  if(!pixel)return 0;
  unsigned index=(((tile>>10)&7)<<bpp)|pixel;
  if(asset_x>=0 && asset_x<8192 && asset_y>=0 && asset_y<8192){
    unsigned phase=resource_phase(1,asset_x,asset_y);
    unsigned current=frame.ram[0x1f1a];
    if((frame.stage==4 && (frame.ram[0x1fc9]&64)) ||
       (frame.stage==6 && (frame.ram[0x1fc3]&64)))current+=5;
    const Palette *pal=prepare_palette(phase);
    /* The upload queue also contains ordinary palette animations. Its count
     * does not invalidate this phase's completed native CGRAM colors. */
    if(pal && pal->valid[index] && phase!=current)*color=pal->colors[index];
  }
  unsigned priority=tile&0x2000?(layer==2 && (p->bgmode&8)?15:high[layer]):low[layer];
  return (uint16_t)((priority<<12)|(layer<<8)|index);
}
/* Resolve native tilemap/ROM indirection once per eight-pixel fragment. The
 * live PPU registers still select each scanline, and resource/palette seams
 * remain per pixel. Large tiles and mosaic retain the general native path. */
static void background_line(const Ppu *p,unsigned layer,int y,unsigned left,
                            unsigned width,uint16_t *pixels,int *colors){
  if(PPU_bigTiles(p,layer) || (p->mosaic&(1u<<layer))){
    for(unsigned i=0;i<width;++i){colors[i]=-1;pixels[i]=background(p,layer,(int)i-(int)left,y,&colors[i]);}
    return;
  }
  static const unsigned low[]={8,7,1},high[]={12,11,3};
  unsigned mode=frame.ram[0x1e99],bpp=layer==2?2:4;
  bool canvas=object_canvas(layer);
  bool stream=!canvas && (layer==0 || (layer==1 && (mode==2 || mode==4 || mode==14 || mode==16)));
  bool half_x=layer==1 && (mode==2 || mode==14);
  bool half_y=layer==1 && (mode==2 || mode==16);
  int origin_x=0,origin_y=0;
  if(stream){
    int nx=word(frame.ram+0x1e5d),ny=word(frame.ram+0x1e60);
    if(!layer){
      origin_x=X3Display_ExpandStageScroll((uint16_t)nx,p->hScroll[0])+frame.dx;
      origin_y=X3Display_ExpandStageScroll((uint16_t)ny,p->vScroll[0])+frame.dy;
    }else{
      int sx=word(frame.ram+0x1e9d),sy=word(frame.ram+0x1ea0);
      origin_x=sx+(((p->hScroll[1]-sx+256)&511)-256)+(half_x?((nx+frame.dx)>>1)-(nx>>1):frame.dx);
      origin_y=sy+(((p->vScroll[1]-sy+128)&255)-128)+(half_y?((ny+frame.dy)>>1)-(ny>>1):frame.dy);
    }
  }
  unsigned current_palette=frame.ram[0x1f1a];
  if((frame.stage==4 && (frame.ram[0x1fc9]&64)) ||
     (frame.stage==6 && (frame.ram[0x1fc3]&64)))current_palette+=5;
  for(unsigned i=0;i<width;){
    int x=(int)i-(int)left,px=(x+p->hScroll[layer])&1023,py=(y+p->vScroll[layer])&1023;
    unsigned tile=0,span;bool valid=true;
    if(canvas){int nx=x+frame.dx,ny=y+frame.dy;
      px=(nx+p->hScroll[layer])&1023;py=(ny+p->vScroll[layer])&1023;valid=canvas_contains(nx,ny);
    }
    if(stream){
      px=origin_x+x;py=origin_y+y;
      if(x<0 && px<0)px=((px%32)+32)%32;
      valid=stage_tile(layer,px,py,&tile);
    }else{
      unsigned sc=p->bgXsc[layer],tx=(unsigned)px>>3,ty=(unsigned)py>>3;
      unsigned a=(sc&0xfc)*256+(tx&31)+(ty&31)*32;
      if((sc&1) && (tx&32))a+=1024;
      if((sc&2) && (ty&32))a+=(sc&1)?2048:1024;
      tile=p->vram[a&32767];
    }
    span=8-((unsigned)px&7);if(span>width-i)span=width-i;
    if(canvas){int begin=-frame.dx,end=256-frame.dx;
      if(x<begin && x+(int)span>begin)span=(unsigned)(begin-x);
      if(x<end && x+(int)span>end)span=(unsigned)(end-x);
    }
    /* The finite-origin margin wrap ends at the native field boundary. */
    if(x<0 && span>(unsigned)-x)span=(unsigned)-x;
    if(layer==2 && x<256 && x+(int)span>256)span=256-x;
    if(layer==2 && (x<0 || x>=256) &&
       !(p->screenEnabled[1]==4 && !(p->screenEnabled[0]&4)))valid=false;
    if(!valid){for(unsigned j=0;j<span;++j){pixels[i+j]=0;colors[i+j]=-1;}i+=span;continue;}
    unsigned cy=(unsigned)py&7;if(tile&0x8000)cy=7-cy;
    unsigned a=(PPU_bgTileAdr(p,layer)+(tile&1023)*bpp*4)&32767;
    uint16_t live[2]={p->vram[(a+cy)&32767],bpp==4?p->vram[(a+8+cy)&32767]:0};
    unsigned priority=tile&0x2000?(layer==2 && (p->bgmode&8)?15:high[layer]):low[layer];
    unsigned prefix=(priority<<12)|(layer<<8),palette=((tile>>10)&7)<<bpp;
    int asset_y=stream?(half_y?py*2-112:py):-1;
    unsigned last_phase=UINT_MAX;const uint8_t *bits=NULL;
    for(unsigned j=0;j<span;++j){
      int asset_x=stream?(half_x?(px+(int)j)*2-128:px+(int)j):-1;
      bool asset=asset_x>=0 && asset_x<8192 && asset_y>=0 && asset_y<8192;
      if(asset){
        unsigned phase=resource_phase(0,asset_x,asset_y);
        if(phase!=last_phase){
          last_phase=phase;prepare_chr(phase);bits=NULL;
          if(phase<16 && chr_valid[phase][a/16] &&
             (phase!=frame.ram[0x1f19] || frame.ram[0x50]))bits=chr[phase]+a*2+cy*2;
        }
      }else{bits=NULL;last_phase=UINT_MAX;}
      unsigned cx=((unsigned)px+j)&7,shift=(tile&0x4000)?cx:7-cx,pixel;
      if(bits)pixel=((bits[0]>>shift)&1)|(((bits[1]>>shift)&1)<<1)|
          (((bits[16]>>shift)&1)<<2)|(((bits[17]>>shift)&1)<<3);
      else{
        pixel=((live[0]>>shift)&1)|(((live[0]>>(8+shift))&1)<<1);
        if(bpp==4)pixel|=(((live[1]>>shift)&1)<<2)|(((live[1]>>(8+shift))&1)<<3);
      }
      colors[i+j]=-1;pixels[i+j]=pixel?(uint16_t)(prefix|palette|pixel):0;
      if(pixel && asset){
        unsigned phase=resource_phase(1,asset_x,asset_y);const Palette *pal=prepare_palette(phase);
        if(pal && pal->valid[palette|pixel] && phase!=current_palette)colors[i+j]=pal->colors[palette|pixel];
      }
    }
    i+=span;
  }
  /* Match the original arena's elastic edge columns without repeating the
   * room, door, moving boss canvas or stale neighboring map into a gutter. */
  if(frame.closed_arena && layer<2){
    int lc=-1,rc=-1;uint16_t lp=0,rp=0;
    if(!layer){lp=background(p,layer,-frame.dx,y,&lc);rp=background(p,layer,255-frame.dx,y,&rc);}
    for(unsigned i=0;i<width;++i){int nx=(int)i-(int)left+frame.dx;
      if(nx<0){pixels[i]=lp;colors[i]=lc;}else if(nx>=256){pixels[i]=rp;colors[i]=rc;}
    }
  }
}
static void world_objects(const Ppu *p,int left,int right){
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  /* The cartridge's descriptor order owns native OBJ overlap. In particular,
   * a leading explosion must remain in front of the primary player's body. */
  unsigned used=0,native_slot=24,primary_slot=24;
  for(unsigned i=0;i<visible->count;++i){
    const X3CoopWorldDraw *draw=&visible->actors[i];const uint8_t *b=draw->body;
    unsigned table,a,n;
    if(!rom_read(0x8d8000+b[0x16]*3,3,&table) ||
       !rom_read(table+(b[0x17]&127)*3,3,&a) || !rom_read(a,1,&n) || n>64)continue;
    unsigned first=native_slot;native_slot+=n;
    if(draw->address==X3_COOP_BODY){if(draw->owner==visible->players.anchor+1)primary_slot=first;continue;}
    if(used==WORLD_PIECES)continue;
    uint16_t colors[256];memcpy(colors,p->cgram,sizeof(colors));
    ObjectResource resource;bool private_resource=object_resource(draw,colors,&resource);
    if(draw->owner){
      const uint8_t *palette=visible->players.players[draw->owner-1].palette;
      for(unsigned j=0;j<32;++j)colors[128+j]=(uint16_t)word(palette+j*2);
    }
    for(unsigned piece=0;piece<n && used<WORLD_PIECES && frame.count<EXTRA_PIECES;++piece){
      unsigned packed;if(!rom_read(a+1+piece*4,4,&packed))break;
      unsigned flags=packed&255,side=flags&32?16:8,attributes=(flags&0xdf)|(b[0x11]&63);
      if(private_resource)attributes=(flags&0xdf)|resource.attributes;
      int x=(int8_t)(packed>>8),y=(int8_t)(packed>>16);
      if(b[0x11]&64){x=-x-(int)side;attributes^=64;}
      if(b[0x11]&128){y=-y-(int)side;attributes^=128;}
      x+=(int)word(b+5)-frame.x;y+=(int)word(b+8)-frame.y;
      if(x+(int)side<=-left || x>=256+right || y+(int)side<=0 || y>=224)continue;
      unsigned tile=((packed>>24)+(private_resource?resource.tile:b[0x18]))&255;
      unsigned base=((attributes&1)?PPU_objTileAdr2(p):PPU_objTileAdr1(p))+tile*16;
      uint16_t *pixels=frame.pixels[used];memset(pixels,0,256*sizeof(*pixels));
      for(unsigned dy=0;dy<side;++dy)for(unsigned dx=0;dx<side;++dx){
        unsigned tx=attributes&64?side-1-dx:dx,ty=attributes&128?side-1-dy:dy;
        unsigned adr=(base+(ty/8)*256+(tx/8)*16)&32767;
        unsigned pixel=private_resource?resource_pixel(&resource,adr,tx&7,ty&7):tile_pixel(p->vram,adr,tx&7,ty&7,4);
        if(pixel)pixels[dy*16+dx]=(uint16_t)(0x8000|colors[128+((attributes>>1)&7)*16+pixel]);
      }
      frame.objects[frame.count++]=(PpuExtraObject){.x=x,.y=y,.width=(uint16_t)side,.height=(uint16_t)side,.stride=16,
        .priority=(attributes>>4)&3,.math=((attributes>>1)&7)>=4,
        .oamSlot=(uint8_t)(first+piece<128?first+piece:127),.order=first+piece<128?1:2+first+piece-128,.pixels=pixels};
      ++used;
    }
  }
  for(unsigned seat=0;seat<2 && frame.count<EXTRA_PIECES;++seat){
    const PpuExtraObject *body=X3CoopRenderPlayer(seat,frame.x,frame.y);
    if(body){PpuExtraObject object=*body;
      if(seat==visible->players.anchor){object.oamSlot=(uint8_t)(primary_slot<128?primary_slot:127);object.order=primary_slot<128?1:2+primary_slot-128;}
      else{object.oamSlot=108;object.order=2;} /* Native counterpart follows an original OBJ on a tie. */
      frame.objects[frame.count++]=object;}
  }
}
static void boss_meter(const Ppu *p,int right){
  /* These are the seven native right-side meter slots, also recognized by
   * the solo widescreen renderer. Do not show them in a remote room. */
  if(frame.dx< -96 || frame.dx>96 || frame.dy< -64 || frame.dy>64 || PPU_objSize(p)!=0)return;
  unsigned matches=0;
  for(unsigned i=16;i<23;++i){
    unsigned xy=p->oam[2*i],tile=p->oam[2*i+1];
    unsigned high=(p->highOam[i>>2]>>((i&3)*2))&3;
    matches+=(xy&255)==232 && (xy>>8)<112 && (tile>>8)==0x34 && high==2;
  }
  if(matches<4)return;
  for(unsigned i=16;i<23 && frame.count<EXTRA_PIECES;++i){
    unsigned xy=p->oam[2*i],tile=p->oam[2*i+1];
    unsigned high=(p->highOam[i>>2]>>((i&3)*2))&3;
    if((xy&255)!=232 || (xy>>8)>=112 || (tile>>8)!=0x34 || high!=2)continue;
    uint16_t *pixels=frame.boss_hud[i-16];
    unsigned address=PPU_objTileAdr1(p)+(tile&255)*16;
    for(unsigned y=0;y<16;++y)for(unsigned x=0;x<16;++x){
      unsigned adr=(address+(y/8)*256+(x/8)*16)&32767;
      unsigned ink=tile_pixel(p->vram,adr,x&7,y&7,4);
      pixels[y*16+x]=ink?(uint16_t)(0x8000|p->cgram[160+ink]):0;
    }
    frame.objects[frame.count++]=(PpuExtraObject){.x=232+right,.y=(int)(xy>>8),.width=16,.height=16,.stride=16,
      .priority=3,.oamSlot=(uint8_t)i,.order=1,.pixels=pixels};
  }
}
static bool begin_view(Ppu *p,unsigned seat,bool shared){
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  if(!p || !p->renderBuffer || seat>1 || !X3CoopRuntimeReady() || (!shared && !X3CoopViewsIndependent()) ||
     !visible->players.initialized || visible->players.wipe || !g_snes || !g_snes->cart ||
     g_ram[0xd1]!=4 || g_ram[0xd2]!=4 || (X3CoopNativePauseActive(g_ram) && !X3CoopMenuGameplayFade()) || (p->bgmode&7)!=1)return false;
  active_frame=&independent_frames[seat];
  frame.rom=g_snes->cart->rom;frame.size=g_snes->cart->romImageSize;
  memcpy(frame.ram,g_ram,sizeof(frame.ram));
  frame.x=shared?visible->camera[0]:visible->views[seat][0];
  frame.y=shared?visible->camera[1]:visible->views[seat][1];
  frame.dx=frame.x-(int)visible->camera[0];frame.dy=frame.y-(int)visible->camera[1];
  frame.ram[0x1e5d]=(uint8_t)visible->camera[0];frame.ram[0x1e5e]=(uint8_t)(visible->camera[0]>>8);
  frame.ram[0x1e60]=(uint8_t)visible->camera[1];frame.ram[0x1e61]=(uint8_t)(visible->camera[1]>>8);
  frame.stage=frame.ram[0x1fae];frame.count=0;
  const X3FixedArena *arena=X3FixedArenaFor(frame.ram);
  frame.closed_arena=arena && !arena->world_right && !frame.dx && !frame.dy && (p->bgXsc[0]&0xfc)==0x50 && (p->bgXsc[1]&0xfc)==0x58;
  if(frame.stage>14)return false;
  if(cache_stage!=frame.stage || cache_rom!=frame.rom){
    if(cache_rom!=frame.rom)memset(file_ready,0,sizeof(file_ready));
    cache_stage=frame.stage;cache_rom=frame.rom;
    memset(chr_ready,0,sizeof(chr_ready));memset(chr_valid,0,sizeof(chr_valid));
    memset(palette_ready,0,sizeof(palette_ready));memset(palettes,0,sizeof(palettes));
  }
  int extra=(int)(panel_width-256)/2,left=extra?extra:p->extraLeftCur,right=extra?extra:p->extraRightCur;
  prepare_events();world_objects(p,left,right);boss_meter(p,right);
  unsigned layout=X3CoopHudLayout();
  for(unsigned who=shared?0:seat;who<(shared?2:seat+1) && frame.count<EXTRA_PIECES;++who){
    const X3CoopPlayer *actor=&visible->players.players[who];
    if(shared && actor->life==X3_COOP_ABSENT)continue;
    bool inverted=shared && layout==X3_COOP_HUD_COMPACT && actor->character==X3_COOP_ZERO;
    X3CoopHudPlayer(p,visible,who,frame.rom,frame.size,inverted,frame.hud[who]);
    frame.objects[frame.count++]=(PpuExtraObject){.x=8-left+(shared && layout==X3_COOP_HUD_HORIZONTAL?(int)who*32:0),.y=inverted?100:0,
      .width=X3_COOP_HUD_WIDTH,.height=X3_COOP_HUD_HEIGHT,.stride=X3_COOP_HUD_WIDTH,
      .priority=3,.oamSlot=0,.order=who+1,.pixels=frame.hud[who]};
  }
  return true;
}
bool X3Display_BeginIndependentFrame(Ppu *p){
  rendered_private=false;
  int seat=X3Display_PresentationSeat();
  if(seat>=0 && X3CoopViewsIndependent())return begin_view(p,(unsigned)seat,false);
  if(X3CoopViewsIndependent()){
    /* Narrow Shared needs the same per-room armor art as wider/split views;
     * native OAM alone can still point at the other region's shared tiles. */
    const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
    for(unsigned i=0;i<visible->count;++i)if(visible->actors[i].address==0xcc8 && !visible->actors[i].body[10])
      return begin_view(p,0,true);
  }
  return panel_width>256 && begin_view(p,0,true);
}
bool X3Display_BeginLocalSplitFrame(Ppu *p){
  rendered_private=false;
  if(!X3Display_LocalSplit() || !p || p->extraLeftRight)return false;
  return begin_view(p,0,false) && begin_view(p,1,false);
}
static bool window(const Ppu *p,unsigned layer,int x,int extra){
  unsigned flags=(p->windowsel>>(layer*4))&15;
  int l1=p->window1left==0?-extra:p->window1left,r1=p->window1right==255?255+extra:p->window1right;
  int l2=p->window2left==0?-extra:p->window2left,r2=p->window2right==255?255+extra:p->window2right;
  bool a=(x>=l1 && x<=r1)!=((flags&1)!=0),b=(x>=l2 && x<=r2)!=((flags&4)!=0);
  if(!(flags&2))return (flags&8) && b;
  if(!(flags&8))return a;
  switch((p->wbgobjlog>>(layer*2))&3){case 0:return a||b;case 1:return a&&b;case 2:return a!=b;default:return a==b;}
}
static bool condition(unsigned mode,bool inside){return mode==3 || (mode==1 && !inside) || (mode==2 && inside);}
static unsigned resolve_color(const Ppu *p,uint16_t pixel,int object,const int backgrounds[3]){
  unsigned layer=(pixel>>8)&15;
  if((layer==4 || layer==6) && object>=0)return (unsigned)object&32767;
  return layer<3 && backgrounds[layer]>=0?(unsigned)backgrounds[layer]:p->cgram[pixel&255];
}
static uint32_t color(const Ppu *p,uint16_t main,uint16_t sub,bool inside,int object,const int backgrounds[3]){
  unsigned rgb=resolve_color(p,main,object,backgrounds),layer=(main>>8)&15;
  bool clipped=condition(p->cgwsel>>6,inside);
  bool math=!condition((p->cgwsel>>4)&3,inside) && ((p->cgadsub&63)&(1u<<layer));
  unsigned other=p->fixedColor;bool half=math && (p->cgadsub&64) && !clipped;
  if(math && (p->cgwsel&2)){if(sub&255)other=resolve_color(p,sub,object,backgrounds);else half=false;}
  uint32_t result=0;
  for(unsigned component=0;component<3;++component){
    int c=clipped?0:(rgb>>(component*5))&31;
    if(math){int second=(other>>(component*5))&31;c+=p->cgadsub&128?-second:second;
      if(c<0)c=0;if(half)c/=2;if(c>31)c=31;}
    result|=(uint32_t)p->brightnessMult[c]<<(16-component*8);
  }
  return result;
}
static void draw_view_line(const Ppu *p,int line,uint32_t *out,unsigned budget,unsigned left,unsigned right){
  unsigned width=256+left+right,extra=(budget-256)/2;
  if(width>kPpuBufWidth || left>extra || right>extra)return;
  memset(out,0,budget*sizeof(*out));
  rendered_private=true;
  if(p->inidisp&128)return;
  out+=extra-left;
  uint16_t objects[kPpuBufWidth],colors[kPpuBufWidth];uint64_t order[kPpuBufWidth];
  for(unsigned i=0;i<width;++i){objects[i]=0x500;colors[i]=0;order[i]=UINT64_MAX;}
  PpuComposeExtraObjects(frame.objects,frame.count,line-1,-(int)left,width,
      PPU_objPriority(p)?(p->oamaddl&254)>>1:0,objects,colors,order);
  static int tile_rows=-1;
  if(tile_rows<0){const char *value=getenv("SNESRECOMP_X3_COOP_TILE_ROWS");tile_rows=value && value[0]=='0'?0:value && value[0]=='v'?2:1;}
  uint16_t backgrounds[3][kPpuBufWidth];int background_colors[3][kPpuBufWidth];
  if(tile_rows)for(unsigned layer=0;layer<3;++layer)
    background_line(p,layer,line,left,width,backgrounds[layer],background_colors[layer]);
  for(unsigned i=0;i<width;++i){
    int x=(int)i-(int)left,bg_colors[3]={-1,-1,-1};uint16_t layers[4];
    for(unsigned layer=0;layer<3;++layer){
      if(tile_rows){
        layers[layer]=backgrounds[layer][i];bg_colors[layer]=background_colors[layer][i];
        if(tile_rows==2){
          int original_color=-1;uint16_t original=background(p,layer,x,line,&original_color);
          if(original!=layers[layer] || original_color!=bg_colors[layer]){
            fprintf(stderr,"X3 tile row mismatch stage%u seat%u layer%u xy%d,%d pixel%04x/%04x color%d/%d\n",frame.stage,active_frame==&independent_frames[1],layer,x,line,original,layers[layer],original_color,bg_colors[layer]);abort();
          }
        }
      }
      else layers[layer]=background(p,layer,x,line,&bg_colors[layer]);
    }
    layers[3]=objects[i];
    uint16_t screen[2]={0x500,0x500};
    for(unsigned s=0;s<2;++s)for(unsigned layer=0;layer<4;++layer){
      unsigned bit=layer==3?16:1u<<layer,wl=layer==3?4:layer;
      if((p->screenEnabled[s]&bit) && (!(p->screenWindowed[s]&bit) ||
          !window(p,wl,x,left)) && layers[layer]>screen[s])screen[s]=layers[layer];
    }
    out[i]=color(p,screen[0],screen[1],window(p,5,x,left),colors[i]?colors[i]:-1,bg_colors);
  }
}
void X3Display_DrawIndependentLine(Ppu *p,int line){
  if(line<1 || line>224)return;
  uint8_t *native=p->renderBuffer+(size_t)(line-1)*p->renderPitch;
  if(panel_width>256){
    unsigned extra=(panel_width-256)/2;
    uint32_t *out=coop_pixels+(size_t)(line-1)*panel_width;
    draw_view_line(p,line,out,panel_width,extra,extra);
    memcpy(native,out+extra,256*4);
    if(line==224){composed_width=panel_width;composed_split=false;}
  }else{
    unsigned budget=256+p->extraLeftRight*2;
    if(budget*4<=p->renderPitch)
      draw_view_line(p,line,(uint32_t *)native,budget,p->extraLeftCur,p->extraRightCur);
  }
}
void X3Display_DrawLocalSplitLine(Ppu *p,int line){
  if(line<1 || line>224)return;
  uint32_t *row=coop_pixels+(size_t)(line-1)*2*panel_width;
  unsigned extra=(panel_width-256)/2;
  for(unsigned seat=0;seat<2;++seat){
    active_frame=&independent_frames[seat];
    draw_view_line(p,line,row+seat*panel_width,panel_width,extra,extra);
  }
  /* Keep the host's primary native field coherent for states and rewind. */
  memcpy(p->renderBuffer+(size_t)(line-1)*p->renderPitch,row+extra,256*4);
  if(line==224){composed_width=2*panel_width;composed_split=true;}
}
void X3Display_CaptureCoopNative(const Ppu *p){
  bool split=X3Display_LocalSplit();
  if((!split && panel_width==256) || !p || !p->renderBuffer)return;
  /* Menus, arrivals and shared scripts retain their real native scanout. */
  unsigned seats=split?2:1,width=panel_width*seats,extra=(panel_width-256)/2;
  for(unsigned y=0;y<224;++y){
    uint32_t *row=coop_pixels+(size_t)y*width;
    memset(row,0,width*4);
    for(unsigned seat=0;seat<seats;++seat)
      memcpy(row+seat*panel_width+extra,p->renderBuffer+(size_t)y*p->renderPitch,256*4);
  }
  X3CoopMenuDraw(p,(uint8_t *)coop_pixels,width*4,width,split?-1:X3Display_PresentationSeat());
  composed_width=width;composed_split=split;
}
int X3Display_DrawCoopFrame(uint8_t *dst,size_t pitch,const uint8_t *field,
                            int width,int height,double alpha){
  (void)alpha;
  bool split=X3Display_LocalSplit();
  unsigned composed=panel_width*(split?2:1);
  if((!split && panel_width==256) || !dst || width!=(int)composed || height!=224 || pitch<composed*4)return 0;
  for(unsigned y=0;y<224;++y){
    uint8_t *row=dst+(size_t)y*pitch;
    if(composed_width==composed && composed_split==split){memcpy(row,coop_pixels+(size_t)y*composed,composed*4);continue;}
    /* A resumed paused state supplies a real native thumbnail before there
     * is a new co-op raster. Show it until the first field at this width. */
    memset(row,0,composed*4);
    if(field)for(unsigned seat=0;seat<(split?2u:1u);++seat)
      memcpy(row+(seat*panel_width+(panel_width-256)/2)*4,field+(size_t)y*256*4,256*4);
  }
  return 1;
}
