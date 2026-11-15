
#include "snes/cx4.h"
#include "../src/x3_fixed_arena.h"

/* Exercise the pure per-view composition entrypoints. X3DrawPpuFrame also
 * advances native PPU field/HDMA bookkeeping once per simulation field;
 * calling it once for each proposed view would test extra native scanouts. */
static void cx4_picture(uint8_t *pixels,unsigned seat,unsigned panel,bool split){
  X3Display_SetPresentationSeat(seat<2?(int)seat:-1);X3Display_SetLocalSplit(split);X3Display_SetCoopPanelWidth(panel);
  check(split?X3Display_BeginLocalSplitFrame(g_ppu):X3Display_BeginIndependentFrame(g_ppu),"actual Cx4 per-view composition begins");
  for(int line=1;line<=224;++line){if(split)X3Display_DrawLocalSplitLine(g_ppu,line);else X3Display_DrawIndependentLine(g_ppu,line);}
  unsigned width=panel*(split?2:1);
  if(split || panel>256)check(X3Display_DrawCoopFrame(pixels,width*4,NULL,width,224,1),"actual complete Cx4 co-op field presents");
  else memcpy(pixels,g_ppu->renderBuffer,256u*224u*4u);
}

/* Independent source oracle: the original PPU's already-qualified finite
 * arena policy. Isolate BG1 in both copies; preserve all actual source tiles,
 * scrolls, palettes, windows and math. The detached PPU executes no guest code. */
static void native_arena_edge_picture(unsigned panel,unsigned anchor,uint8_t *private,uint8_t *native){
  const X3FixedArena *arena=X3FixedArenaFor(g_ram);if(!arena || arena->world_right)return;
  Ppu *copy=malloc(sizeof(*copy));check(copy!=NULL,"original finite-arena PPU copy allocates");
  uint8_t screens[2]={g_ppu->screenEnabled[0],g_ppu->screenEnabled[1]};
  g_ppu->screenEnabled[0]=1;g_ppu->screenEnabled[1]=0;*copy=*g_ppu;
  copy->hdFrame=NULL;copy->hdCapture=NULL;PpuClearOverlayBindings(copy);
  PpuSetExtraSpace(copy,(uint16_t)((panel-256)/2));
  PpuSetWidescreenLayerElasticSplitBandSlot(copy,0,0,0,225,0,1,255,256);
  PpuSetWidescreenLayerClamp(copy,2);
  PpuBeginDrawing(copy,native,panel*4,kPpuRenderFlags_NewRenderer);
  PpuDrawDetachedLine(copy,0);for(int line=1;line<=224;++line)PpuDrawDetachedLine(copy,line);
  cx4_picture(private,anchor,panel,false);
  unsigned mismatch=0,positive=0,extra=(panel-256)/2;
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<panel;++x){unsigned at=(y*panel+x)*4;
    if(memcmp(native+at,private+at,3)){if(mismatch<4)fprintf(stderr,"native ARENA PIXEL %u,%u rgb%02x%02x%02x/%02x%02x%02x\n",x,y,native[at],native[at+1],native[at+2],private[at],private[at+1],private[at+2]);++mismatch;}
    if(x<extra || x>=extra+256)positive|=native[at]!=native[(y*panel+extra+128)*4];
  }
  fprintf(stderr,"native ARENA EDGE width%u differences%u actualEdgeDetail%u\n",panel,mismatch,positive);
  const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");if(out && *out){char path[1024];snprintf(path,sizeof(path),"%s-arena-BG1-%u-native.ppm",out,panel);WritePpm(path,native,panel*4,panel,224,0,false);snprintf(path,sizeof(path),"%s-arena-BG1-%u-private.ppm",out,panel);WritePpm(path,private,panel*4,panel,224,0,false);}
  g_ppu->screenEnabled[0]=screens[0];g_ppu->screenEnabled[1]=screens[1];free(copy);
  check(positive && !mismatch,"the complete wide private BG1 matches the original native arena's actual edge policy");
}
/* Existing earned encounter states; no boss/result/graphics state is supplied.
 * Actor protection is only a prerequisite for observing the native animation. */
static void native_cx4_presentation_checks(const char *fixture,uint8_t *cut,uint8_t *left,uint8_t *right,size_t capacity){
  check(RtlLoadSnapshot(fixture) && (g_ram[0x1fae]==0 || g_ram[0x1fae]==12 || g_ram[0x1fae]==13),"Cx4 presentation loads its actual earned native boss encounter");X3CoopRuntimeFrame();
  check(g_snes->cart->cx4 && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual Cx4 encounter retains both living native participants");
  size_t n=RtlSaveSnapshotToMemory(cut,capacity);X3CoopState setup=*X3CoopRuntimeState();for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x88]=255;
  check(X3CoopEncode(&setup,cut+contact_context_offset(cut,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(cut,n),"Cx4 scene supplies only participant damage protection");door_install_player(&setup);X3CoopViewsConfigure(true);
  bool wide=g_config.widescreen;g_config.widescreen=false;X3Display_SetCoopPanelWidth(256);X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
  uint8_t *split=malloc(512u*224u*4u);check(split!=NULL,"Cx4 split comparison buffer allocates");
  uint64_t initial=cx4_instructions_executed(g_snes->cart->cx4);unsigned cuts=0;
  for(unsigned f=0;f<128;++f){frame(0);
    if(f!=3 && f!=31 && f!=63 && f!=95)continue;
    check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"both actual Cx4 views remain assigned to living participants");
    n=RtlSaveSnapshotToMemory(cut,capacity);char tag[64];snprintf(tag,sizeof(tag),"cx4-field%u",f);native_display_checks(tag,cut,n,left,right,capacity);
    size_t raster=RtlNetplaySnapshotToMemory(cut,capacity);
    uint64_t clock=g_cpu.master_cycles,instructions=cx4_instructions_executed(g_snes->cart->cx4);unsigned runs=cx4_run_ring_count(g_snes->cart->cx4);
    cx4_picture(left,0,256,false);cx4_picture(right,1,256,false);cx4_picture(split,2,256,true);
    for(unsigned y=0;y<224;++y)check(!memcmp(split+y*512*4,left+y*256*4,256*4) && !memcmp(split+(y*512+256)*4,right+y*256*4,256*4),"both actual Cx4 split fields exactly equal their selected native-owner compositions");
    if(g_ram[0x1e99]==14 || (getenv("X3_COOP_ARENA_EDGE_REFERENCE") && X3FixedArenaFor(g_ram))){
      const unsigned widths[]={342,448,800};unsigned anchor=X3CoopRuntimeState()->anchor;
      for(unsigned wi=0;wi<3;++wi){unsigned panel=widths[wi],extra=(panel-256)/2;
        cx4_picture(left,anchor,panel,false);uint8_t screens[2]={g_ppu->screenEnabled[0],g_ppu->screenEnabled[1]};
        g_ppu->screenEnabled[0]&=(uint8_t)~2;g_ppu->screenEnabled[1]&=(uint8_t)~2;cx4_picture(right,anchor,panel,false);
        g_ppu->screenEnabled[0]=screens[0];g_ppu->screenEnabled[1]=screens[1];unsigned inside=0,outside=0;
        for(unsigned y=0;y<224;++y)for(unsigned x=0;x<panel;++x)if(memcmp(left+(y*panel+x)*4,right+(y*panel+x)*4,3)){if(x>=extra && x<extra+256)++inside;else ++outside;}
        check(!outside,"the real live Cx4 body canvas never echoes into wide margins");
        if(f==3){bool kaiser=false;for(unsigned d=0xd18;d<0x10d8;d+=64)kaiser|=g_ram[d] && g_ram[d+10]==0x5f && (g_ram[d+0x27]&127);if(kaiser)check(inside>100,"wide canvas clipping retains the actual nonempty native boss body");}
        const char *output=getenv("X3_COOP_DISPLAY_OUTPUT");if(output && *output && panel==800 && f==3){char path[1024];snprintf(path,sizeof(path),"%s-%s-wide800.ppm",output,tag);WritePpm(path,left,panel*4,panel,224,0,false);}
        fprintf(stderr,"native CX4 CANVAS field%u width%u bodyPixels%u outside%u\n",f,panel,inside,outside);
        if(getenv("X3_COOP_ARENA_EDGE_REFERENCE"))native_arena_edge_picture(panel,anchor,left,right);
      }
    }
    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3Display_SetCoopPanelWidth(256);
    size_t untouched=RtlNetplaySnapshotToMemory(right,capacity);
    same(cut,RtlNetplaySnapshotHashedBytes(raster),right,RtlNetplaySnapshotHashedBytes(untouched),"rendering every Cx4 view leaves the entire canonical simulation state unchanged");
    check(clock==g_cpu.master_cycles && instructions==cx4_instructions_executed(g_snes->cart->cx4) && runs==cx4_run_ring_count(g_snes->cart->cx4),"composing both Cx4 views executes no additional CPU or DSP work");
    const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");if(out && *out){char path[1024];snprintf(path,sizeof(path),"%s-%s-split.ppm",out,tag);WritePpm(path,split,512*4,512,224,0,false);}
    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);++cuts;
  }
  check(cuts==4 && cx4_instructions_executed(g_snes->cart->cx4)>initial,"the observed original encounter really executes its Cx4 program across four animation cuts");
  fprintf(stderr,"native CX4 PRESENTATION cuts%u originalPPU/private world+meters exact bothViews/split exact DSPbefore%llu after%llu noRenderUpdates\n",cuts,(unsigned long long)initial,(unsigned long long)cx4_instructions_executed(g_snes->cart->cx4));
  free(split);g_config.widescreen=wide;
}
