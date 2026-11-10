/* X3 independent views over the native world streams, resource controllers
 * and completed Cx4 draw submissions.
 * Rendering never selects a player or modifies guest memory. */
#include "x3_display.h"
#include "x3_coop_runtime.h"
#include "x3_coop_present.h"
#include "x3_coop_render.h"
#include "x3_coop_view.h"
#include "x3_coop_hud.h"
#include "x3_native_resources.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/cart.h"
#include <string.h>
#include <limits.h>
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
  ResourceEvent event[256];
  uint8_t phase[3][8192];
  PpuExtraObject objects[EXTRA_PIECES];
  uint16_t pixels[WORLD_PIECES][256];
  uint16_t hud[X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT];
} IndependentFrame;
static IndependentFrame independent_frames[2];
static IndependentFrame *active_frame=&independent_frames[0];
#define frame (*active_frame)
static uint8_t split_pixels[512*224*4];
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
  if(draw->owner || !b[10])return false;
  /* The common native02:E15C constructor selects group/file from06:E28E.
   * A customized group is left on its original native path. */
  unsigned group,key;
  if(draw->address>=0xd18 && draw->address<0x10d8){
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
  if(!hologram && phase==frame.ram[0x1f18] && b[0x18]==out->tile && (b[0x11]&63)==out->attributes)return false;
  if(!file_ready[key]){
    file_ready[key]=X3ResourceDecode(frame.rom,frame.size,key,files[key],sizeof(files[key]),&file_length[key])?1:2;
  }
  if(file_ready[key]!=1 || !X3ResourceLayout(frame.rom,frame.size,key,target,out->segments,&out->count) ||
     !X3ResourcePalette(frame.rom,frame.size,palette,offset,colors))return false;
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
static uint16_t background(const Ppu *p,unsigned layer,int x,int y,int *color){
  static const unsigned low[]={8,7,1},high[]={12,11,3};
  unsigned bpp=layer==2?2:4,size=PPU_bigTiles(p,layer)?16:8;
  if(p->mosaic&(1u<<layer)){
    int m=(p->mosaic>>4)+1;
    int mod=x%m;if(mod<0)mod+=m;x-=mod;
    y-=(y-p->mosaicStartLine)%m;
  }
  int px=(x+p->hScroll[layer])&1023,py=(y+p->vScroll[layer])&1023;
  unsigned sc=p->bgXsc[layer],tx=(unsigned)px/size,ty=(unsigned)py/size;
  unsigned a=(sc&0xfc)*256+(tx&31)+(ty&31)*32;
  if((sc&1) && (tx&32))a+=1024;
  if((sc&2) && (ty&32))a+=(sc&1)?2048:1024;
  unsigned tile=p->vram[a&32767];int asset_x=-1,asset_y=-1;
  unsigned mode=frame.ram[0x1e99];
  bool stream=layer==0 || (layer==1 && (mode==2 || mode==4 || mode==14 || mode==16));
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
static void world_objects(const Ppu *p){
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  unsigned used=0;
  for(unsigned i=0;i<visible->count && used<WORLD_PIECES;++i){
    const X3CoopWorldDraw *draw=&visible->actors[i];const uint8_t *b=draw->body;
    if(draw->address==X3_COOP_BODY)continue;
    unsigned table,a,n;
    if(!rom_read(0x8d8000+b[0x16]*3,3,&table) ||
       !rom_read(table+(b[0x17]&127)*3,3,&a) || !rom_read(a,1,&n) || n>64)continue;
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
      if(x+(int)side<=-(int)p->extraLeftCur || x>=256+p->extraRightCur || y+(int)side<=0 || y>=224)continue;
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
        .priority=(attributes>>4)&3,.math=((attributes>>1)&7)>=4,.oamSlot=(uint8_t)(16+used),.order=1,.pixels=pixels};
      ++used;
    }
  }
  for(unsigned seat=0;seat<2 && frame.count<EXTRA_PIECES;++seat){
    const PpuExtraObject *body=X3CoopRenderPlayer(seat,frame.x,frame.y);
    if(body){PpuExtraObject object=*body;object.oamSlot=seat==visible->players.anchor?8:108;
      frame.objects[frame.count++]=object;}
  }
}
static bool begin_independent_frame(Ppu *p,unsigned seat){
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  if(!p || !p->renderBuffer || seat>1 || !X3CoopRuntimeEnabled() || !X3CoopViewsIndependent() ||
     !visible->players.initialized || visible->players.wipe || !g_snes || !g_snes->cart ||
     g_ram[0xd1]!=4 || g_ram[0xd2]!=4 || (p->bgmode&7)!=1)return false;
  active_frame=&independent_frames[seat];
  frame.rom=g_snes->cart->rom;frame.size=g_snes->cart->romImageSize;
  memcpy(frame.ram,g_ram,sizeof(frame.ram));
  frame.x=visible->views[seat][0];frame.y=visible->views[seat][1];
  frame.dx=frame.x-(int)visible->camera[0];frame.dy=frame.y-(int)visible->camera[1];
  frame.ram[0x1e5d]=(uint8_t)visible->camera[0];frame.ram[0x1e5e]=(uint8_t)(visible->camera[0]>>8);
  frame.ram[0x1e60]=(uint8_t)visible->camera[1];frame.ram[0x1e61]=(uint8_t)(visible->camera[1]>>8);
  frame.stage=frame.ram[0x1fae];frame.count=0;
  if(frame.stage>14)return false;
  if(cache_stage!=frame.stage || cache_rom!=frame.rom){
    if(cache_rom!=frame.rom)memset(file_ready,0,sizeof(file_ready));
    cache_stage=frame.stage;cache_rom=frame.rom;
    memset(chr_ready,0,sizeof(chr_ready));memset(chr_valid,0,sizeof(chr_valid));
    memset(palette_ready,0,sizeof(palette_ready));memset(palettes,0,sizeof(palettes));
  }
  prepare_events();world_objects(p);
  if(frame.count<EXTRA_PIECES){
    const X3CoopPlayer *player=&visible->players.players[seat];
    X3CoopHudHealth(p,player,player->maximum_health,frame.rom,frame.size,frame.hud);
    frame.objects[frame.count++]=(PpuExtraObject){.x=8-(int)p->extraLeftCur,.y=0,
      .width=X3_COOP_HUD_WIDTH,.height=X3_COOP_HUD_HEIGHT,.stride=X3_COOP_HUD_WIDTH,
      .priority=3,.oamSlot=0,.order=1,.pixels=frame.hud};
  }
  return true;
}
bool X3Display_BeginIndependentFrame(Ppu *p){
  rendered_private=false;
  int seat=X3Display_PresentationSeat();
  return seat>=0 && begin_independent_frame(p,(unsigned)seat);
}
bool X3Display_BeginLocalSplitFrame(Ppu *p){
  rendered_private=false;
  if(!X3Display_LocalSplit() || !p || p->extraLeftRight)return false;
  return begin_independent_frame(p,0) && begin_independent_frame(p,1);
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
void X3Display_DrawIndependentLine(Ppu *p,int line){
  if(line<1 || line>224)return;
  unsigned width=256+p->extraLeftCur+p->extraRightCur;
  if(width>kPpuBufWidth)return;
  uint32_t *out=(uint32_t*)(p->renderBuffer+(size_t)(line-1)*p->renderPitch);
  unsigned budget=256+p->extraLeftRight*2;
  if(budget*4>p->renderPitch || p->extraLeftCur>p->extraLeftRight || p->extraRightCur>p->extraLeftRight)return;
  memset(out,0,budget*sizeof(*out));
  rendered_private=true;
  if(p->inidisp&128)return;
  out+=p->extraLeftRight-p->extraLeftCur;
  uint16_t objects[kPpuBufWidth],colors[kPpuBufWidth];uint64_t order[kPpuBufWidth];
  for(unsigned i=0;i<width;++i){objects[i]=0x500;colors[i]=0;order[i]=UINT64_MAX;}
  PpuComposeExtraObjects(frame.objects,frame.count,line-1,-(int)p->extraLeftCur,width,
      PPU_objPriority(p)?(p->oamaddl&254)>>1:0,objects,colors,order);
  for(unsigned i=0;i<width;++i){
    int x=(int)i-p->extraLeftCur,bg_colors[3]={-1,-1,-1};uint16_t layers[4];
    for(unsigned layer=0;layer<3;++layer)layers[layer]=background(p,layer,x,line,&bg_colors[layer]);
    layers[3]=objects[i];
    uint16_t screen[2]={0x500,0x500};
    for(unsigned s=0;s<2;++s)for(unsigned layer=0;layer<4;++layer){
      unsigned bit=layer==3?16:1u<<layer,wl=layer==3?4:layer;
      if((p->screenEnabled[s]&bit) && (!(p->screenWindowed[s]&bit) ||
          !window(p,wl,x,p->extraLeftCur)) && layers[layer]>screen[s])screen[s]=layers[layer];
    }
    out[i]=color(p,screen[0],screen[1],window(p,5,x,p->extraLeftCur),colors[i]?colors[i]:-1,bg_colors);
  }
}
void X3Display_DrawLocalSplitLine(Ppu *p,int line){
  if(line<1 || line>224)return;
  uint8_t *native=p->renderBuffer;unsigned pitch=p->renderPitch;
  p->renderPitch=512*4;
  for(unsigned seat=0;seat<2;++seat){
    active_frame=&independent_frames[seat];p->renderBuffer=split_pixels+seat*256*4;
    X3Display_DrawIndependentLine(p,line);
  }
  p->renderBuffer=native;p->renderPitch=pitch;
  /* Keep the host's primary native field coherent for states and rewind. */
  memcpy(native+(size_t)(line-1)*pitch,split_pixels+(size_t)(line-1)*512*4,256*4);
}
void X3Display_CaptureLocalSplitNative(const Ppu *p){
  if(!X3Display_LocalSplit() || !p || !p->renderBuffer)return;
  /* Menus, arrivals and shared scripts retain their real native scanout. */
  for(unsigned y=0;y<224;++y)for(unsigned seat=0;seat<2;++seat)
    memcpy(split_pixels+((size_t)y*512+seat*256)*4,p->renderBuffer+(size_t)y*p->renderPitch,256*4);
}
int X3Display_DrawLocalSplit(uint8_t *dst,size_t pitch,const uint8_t *field,
                            int width,int height,double alpha){
  (void)field;(void)alpha;
  if(!X3Display_LocalSplit() || !dst || width!=512 || height!=224 || pitch<512*4)return 0;
  for(unsigned y=0;y<224;++y)memcpy(dst+(size_t)y*pitch,split_pixels+(size_t)y*512*4,512*4);
  return 1;
}
