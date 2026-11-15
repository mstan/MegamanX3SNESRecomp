
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
static void weather_indoor_picture(Ppu *oracle,uint8_t *native,uint8_t *private,
                                  unsigned seat,const uint16_t *star_colors,unsigned star_count,unsigned field){
  bool phase=false;for(unsigned i=0;i<star_count;++i)phase|=star_colors[i]==g_ppu->cgram[30];
  check(phase,"the shared world's current star color belongs to the original native animation");oracle->cgram[30]=g_ppu->cgram[30];
  for(int line=0;line<=224;++line)PpuDrawDetachedLine(oracle,line);
  uint8_t screens[2]={g_ppu->screenEnabled[0],g_ppu->screenEnabled[1]};
  g_ppu->screenEnabled[0]&=(uint8_t)~16;g_ppu->screenEnabled[1]&=(uint8_t)~16;
  cx4_picture(private,seat,256,false);g_ppu->screenEnabled[0]=screens[0];g_ppu->screenEnabled[1]=screens[1];
  unsigned different=0;for(unsigned y=0;y<224;++y)for(unsigned x=48;x<256;++x)different+=memcmp(native+(y*256+x)*4,private+(y*256+x)*4,3)!=0;
  fprintf(stderr,"native WEATHER SCOPE outsideSeat%u field%u backgroundDifferences%u camera%u,%u BG%02x/%02x math%02x/%02x\n",seat,field,different,X3CoopViewsState()->camera[seat][0],X3CoopViewsState()->camera[seat][1],screens[0],screens[1],g_ppu->cgwsel,g_ppu->cgadsub);
  X3Display_SetPresentationSeat(-1);
  check(!different,"the actor remaining indoors retains its own original non-weather background");
}
static void weather_wide_picture(uint8_t *snow,uint8_t *clear,unsigned seat){
  enum {width=342,extra=43};uint16_t tiles[1024];
  check(PPU_bgTileAdr(g_ppu,2)==0 && PPU_bgTileAdr(g_ppu,0)>=1024 && PPU_bgTileAdr(g_ppu,1)>=1024,"the actual snow CHR is disjoint from the two world backgrounds");
  cx4_picture(snow,seat,width,false);memcpy(tiles,g_ppu->vram,sizeof(tiles));memset(g_ppu->vram,0,sizeof(tiles));
  cx4_picture(clear,seat,width,false);memcpy(g_ppu->vram,tiles,sizeof(tiles));
  unsigned changed[3]={0};for(unsigned y=0;y<224;++y)for(unsigned x=0;x<width;++x)
    changed[x<extra?0:x>=extra+256?2:1]+=memcmp(snow+(y*width+x)*4,clear+(y*width+x)*4,3)!=0;
  fprintf(stderr,"native WEATHER WIDE seat%u left%u center%u right%u\n",seat,changed[0],changed[1],changed[2]);
  const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");if(out && *out){char path[1024];snprintf(path,sizeof(path),"%s-wide-snow.ppm",out);WritePpm(path,snow,width*4,width,224,0,false);snprintf(path,sizeof(path),"%s-wide-clear-control.ppm",out);WritePpm(path,clear,width*4,width,224,0,false);}
  X3Display_SetCoopPanelWidth(256);X3Display_SetPresentationSeat(-1);
  check(changed[0] && changed[1] && changed[2],"actual native snowfall covers both adaptive margins as well as the original field");
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
  bool environment=getenv("X3_COOP_ENVIRONMENT_VISUAL")!=NULL;
  bool weather_onset=getenv("X3_COOP_ENVIRONMENT_OUTDOOR_ONSET")!=NULL;
  const char *separated=getenv("X3_COOP_ENVIRONMENT_OUTDOOR_SEPARATED");unsigned outside_seat=separated?(unsigned)atoi(separated):2;uint8_t *outside_reference=NULL;
  Ppu *outside_oracle=NULL;uint16_t star_colors[64];unsigned star_count=0;
  check(RtlLoadSnapshot(fixture),"native presentation loads its complete existing actor state");
  if(environment && getenv("X3_COOP_ENVIRONMENT_OUTDOOR_ORIGINAL")){
    /* Produce the control with co-op disabled before the original loader.
     * Disabling it only after loading could retain an earlier renderer error. */
    X3CoopRuntimeEnable(false);X3CoopViewsConfigure(false);g_config.widescreen=false;
    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
    stage_request();g_ram[0x1fae]=2;unsigned f=0;
    for(;f<3600;++f){frame(0);if(X3CoopRuntimeEnabled())check(false,"original outdoor loader never reenrolls co-op");if(g_ram[0xd1]==4 && g_ram[0xd2]==4 && !g_ram[0x1f22])break;}
    check(f<3600,"original single-actor loader reaches playable stage2");for(unsigned i=0;i<120;++i)frame(0);
    view_word(X3_COOP_BODY+5,4160);view_word(X3_COOP_BODY+8,879);g_ram[X3_COOP_BODY+0x88]=255;
    X3CoopViewState v={0};v.camera[0][0]=3584;v.camera[0][1]=768;v.current[0][1]=v.limits[0][1]=8192;
    v.current[0][2]=v.current[0][3]=v.limits[0][2]=v.limits[0][3]=768;vehicle_install_view(&v,0);
    for(unsigned i=0;i<192;++i){frame(0);if(X3CoopRuntimeEnabled())check(false,"original outdoor producer remains entirely single actor");}
    check((g_ram[X3_COOP_BODY+0x27]&127) && g_ppu->renderPitch==256*4,"actual original outdoor field contains the living native actor at native width");
    const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");if(out && *out){char path[1024];snprintf(path,sizeof(path),"%s-original-actual.ppm",out);WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);

    }
    fprintf(stderr,"native ORIGINAL OUTDOOR actual camera%u,%u body%u,%u HP%u phases%u/%u/%u BG%02x/%02x math%02x/%02x\n",view_read(0x1e5d),view_read(0x1e60),view_read(X3_COOP_BODY+5),view_read(X3_COOP_BODY+8),g_ram[X3_COOP_BODY+0x27]&127,g_ram[0x1f18],g_ram[0x1f19],g_ram[0x1f1a],g_ppu->screenEnabled[0],g_ppu->screenEnabled[1],g_ppu->cgwsel,g_ppu->cgadsub);return;
  }
  if(environment && getenv("X3_COOP_ENVIRONMENT_ENTRY")){
    unsigned target=(unsigned)atoi(getenv("X3_COOP_ENVIRONMENT_ENTRY"));check(target==2 || target==4 || target==5 || target==6,"environment entry selects an authored stage");
    if(getenv("X3_COOP_ENVIRONMENT_ALTERNATE")){
      check(target==4 || target==6,"alternate environment selects the actual native stage variant");
      /* Prior-stage progress is an explicit setup prerequisite. The original
       * loader and BIT/BVC paths choose every palette, map and weather result. */
      g_ram[target==4?0x1fc9:0x1fc3]|=64;
    }
    stage_request();g_ram[0x1fae]=(uint8_t)target;unsigned elapsed=0;
    for(;elapsed<3600;++elapsed){frame(0);if(X3CoopRuntimeReady() && !X3CoopRuntimeSceneActive() && g_ram[0xd1]==4 && g_ram[0xd2]==4 && g_ram[0x1fae]==target)break;}
    check(elapsed<3600,"the original loader reaches the actual environment's playable entry");for(unsigned f=0;f<120;++f)frame(0);
  }
  check(g_ram[0x1fae]==0 || g_ram[0x1fae]==12 || g_ram[0x1fae]==13 ||
      (environment && (g_ram[0x1fae]==2 || g_ram[0x1fae]==4 || g_ram[0x1fae]==5 || g_ram[0x1fae]==6 || g_ram[0x1fae]==10 || g_ram[0x1fae]==11)),"presentation uses its actual native encounter or environment");X3CoopRuntimeFrame();
  check(g_snes->cart->cx4 && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual Cx4 encounter retains both living native participants");
  size_t n=RtlSaveSnapshotToMemory(cut,capacity);X3CoopState setup=*X3CoopRuntimeState();for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x88]=255;
  check(X3CoopEncode(&setup,cut+contact_context_offset(cut,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(cut,n),"Cx4 scene supplies only participant damage protection");door_install_player(&setup);X3CoopViewsConfigure(true);
  if(environment && getenv("X3_COOP_ENVIRONMENT_OUTDOOR")){
    check(g_ram[0x1fae]==2,"outdoor environment source uses the actual Blizzard Buffalo stage");
    if(separated){
      check(outside_seat<2,"separated weather names the actor remaining in the native indoor entry");
      n=RtlSaveSnapshotToMemory(cut,capacity);memcpy(right,cut,n);X3CoopState solo=*X3CoopRuntimeState();solo.current=solo.anchor=(uint8_t)outside_seat;
      check(X3CoopEncode(&solo,cut+contact_context_offset(cut,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(cut,n),"indoor background control selects its actual original actor");door_install_player(&solo);
      X3CoopViewState entry=*X3CoopViewsState();vehicle_install_view(&entry,outside_seat);X3CoopRuntimeEnable(false);X3CoopViewsConfigure(false);g_config.widescreen=false;X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
      for(unsigned i=0;i<196;++i)frame(0);
      const char *control=getenv("X3_COOP_ENVIRONMENT_STATE_OUTPUT");if(control && *control){char path[1024];snprintf(path,sizeof(path),"%s-indoor-control.sav",control);check(RtlSaveSnapshot(path),"native indoor control exports without a picker");}
      check(view_read(0x1e5d)==entry.camera[outside_seat][0] && view_read(0x1e60)==entry.camera[outside_seat][1] && g_ppu->screenEnabled[0]==0x13 && !g_ppu->screenEnabled[1] && !g_ppu->cgadsub,"original indoor reference retains its actual camera, normal planes and disabled weather math");
      outside_reference=malloc(256u*224u*4u);outside_oracle=malloc(sizeof(*outside_oracle));check(outside_reference && outside_oracle,"native indoor background reference allocates");*outside_oracle=*g_ppu;PpuClearOverlayBindings(outside_oracle);outside_oracle->screenEnabled[0]&=(uint8_t)~16;outside_oracle->screenEnabled[1]&=(uint8_t)~16;outside_oracle->renderBuffer=outside_reference;outside_oracle->renderPitch=256*4;
      /* Native04:BC02 animates star color30 every four fields. Weather's
       * native resource wait pauses that clock. Collect original phases,
       * then compare the two backgrounds at the same actual palette phase. */
      for(unsigned i=0;i<64;++i){star_colors[star_count++]=g_ppu->cgram[30];frame(0);}
      check(RtlLoadSnapshotFromMemory(right,n),"separated weather restores the same initial world after its native indoor reference");
    }
    n=RtlSaveSnapshotToMemory(cut,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
    for(unsigned seat=0;seat<2;++seat){if(seat==outside_seat)continue;unsigned x=weather_onset?3968:separated?4160:4160+seat*24,floor=weather_onset?784:896,foot=setup.players[seat].character==X3_COOP_ZERO?25:17;
      check(terrain_test_kind(x,floor)==0x3b && terrain_test_kind(x-12,floor-1)==0 && terrain_test_kind(x+12,floor-1)==0,"actual outdoor source has full-foot native flat terrain");
      campaign_position_only(&setup.players[seat],x,floor-foot);setup.players[seat].body[0x88]=255;
    }
    check(X3CoopEncode(&setup,cut+contact_context_offset(cut,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(cut,n),"outdoor source changes only positions and personal damage protection");door_install_player(&setup);
    X3CoopViewState views=*X3CoopViewsState();views.valid=views.camera_valid=3;
    for(unsigned seat=0;seat<2;++seat){if(seat==outside_seat)continue;views.camera[seat][0]=3584;views.camera[seat][1]=768;
      views.limits[seat][0]=views.current[seat][0]=0;views.limits[seat][1]=views.current[seat][1]=8192;
      views.limits[seat][2]=views.current[seat][2]=768;views.limits[seat][3]=views.current[seat][3]=768;
    }
    X3CoopViewsSetState(&views);vehicle_install_view(&views,setup.anchor);
    for(unsigned f=0;f<192;++f)frame(0);
    if(weather_onset){
      unsigned weather=outside_seat^1;
      check(separated && X3CoopViewsState()->camera[weather][0]==3840 && g_ppu->screenEnabled[0]==0x13 && g_ppu->screenEnabled[1]==4 && !g_ppu->cgadsub,"the settled original world waits exactly at its strict native weather-entry boundary");
      for(unsigned f=0;f<192;++f){frame(f<8?(uint32_t)SNES_PAD_RIGHT<<(weather*12):0);
        if(f==0 || f==7 || f==15 || f==31 || f==63 || f==95 || f==127 || f==159 || f==191)
          weather_indoor_picture(outside_oracle,outside_reference,left,outside_seat,star_colors,star_count,f);
      }
    }
    fprintf(stderr,"native OUTDOOR source camera%u,%u actors%u,%u/%u,%u HP%u/%u phases%u/%u/%u BG%02x/%02x math%02x/%02x\n",view_read(0x1e5d),view_read(0x1e60),support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8),contact_hp(0),contact_hp(1),g_ram[0x1f18],g_ram[0x1f19],g_ram[0x1f1a],g_ppu->screenEnabled[0],g_ppu->screenEnabled[1],g_ppu->cgwsel,g_ppu->cgadsub);
    check(contact_hp(0) && contact_hp(1) && !X3CoopRuntimeSceneActive(),"outdoor source reaches a living playable native world after its original camera pan");
  }
  const char *entry_output=environment?getenv("X3_COOP_ENVIRONMENT_STATE_OUTPUT"):NULL;
  if(entry_output && *entry_output)check(RtlSaveSnapshot(entry_output),"actual native environment entry exports without a picker");
  bool wide=g_config.widescreen;g_config.widescreen=false;X3Display_SetCoopPanelWidth(256);X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
  uint8_t *split=malloc(512u*224u*4u);check(split!=NULL,"Cx4 split comparison buffer allocates");
  uint64_t initial=cx4_instructions_executed(g_snes->cart->cx4);unsigned cuts=0,math_pixels=0,native_cuts=0;
  for(unsigned f=0;f<128;++f){frame(0);
    if(f!=3 && f!=31 && f!=63 && f!=95)continue;
    if(environment && getenv("X3_COOP_ENVIRONMENT_OUTDOOR"))
      check((outside_seat==0 || (g_ppu->cgram[5]==0x421 && g_ppu->cgram[6]==0x421 && g_ppu->cgram[7]==0x421)) && g_ppu->screenEnabled[0]==4 && g_ppu->screenEnabled[1]==0x13 && g_ppu->cgadsub==0x7f,"actual co-op snowfall stays active with the original native weather planes and math");
    if(environment){uint32_t palette=2166136261u,tiles=2166136261u;
      for(unsigned i=0;i<128;++i)palette=(palette^g_ppu->cgram[i])*16777619u;
      for(unsigned i=0;i<0x6000;++i)tiles=(tiles^g_ppu->vram[i])*16777619u;
      fprintf(stderr,"native ENVIRONMENT stage%u field%u palette%08x BGdata%08x math%02x/%02x fixed%04x source%u/%u/%u alternate%u camera%u,%u\n",g_ram[0x1fae],f,palette,tiles,g_ppu->cgwsel,g_ppu->cgadsub,g_ppu->fixedColor,g_ram[0x1f18],g_ram[0x1f19],g_ram[0x1f1a],(g_ram[g_ram[0x1fae]==4?0x1fc9:0x1fc3]&64)!=0,view_read(0x1e5d),view_read(0x1e60));
      fprintf(stderr,"native ENVIRONMENT LAYERS main/sub%02x/%02x BG3scroll%u/%u BG2scroll%u/%u\n",g_ppu->screenEnabled[0],g_ppu->screenEnabled[1],g_ppu->hScroll[2],g_ppu->vScroll[2],g_ppu->hScroll[1],g_ppu->vScroll[1]);
    }
    check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"both actual Cx4 views remain assigned to living participants");
    n=RtlSaveSnapshotToMemory(cut,capacity);char tag[64];snprintf(tag,sizeof(tag),"cx4-field%u",f);
    /* A shared native PPU can carry snow for the remote participant. The
     * unaffected anchor instead has the actual original indoor control. */
    /* The onset-only roof source has an unqualified native pickup OBJ
     * reference. This source tests weather against the independent original
     * indoor background; the settled sources retain full native OBJ checks. */
    if(!weather_onset && (!separated || outside_seat!=X3CoopRuntimeState()->anchor)){native_display_checks(tag,cut,n,left,right,capacity);++native_cuts;}
    if(outside_reference){
      weather_indoor_picture(outside_oracle,outside_reference,right,outside_seat,star_colors,star_count,192+f);
      const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");if(out && *out){char path[1024];snprintf(path,sizeof(path),"%s-outside-native.ppm",out);WritePpm(path,outside_reference,256*4,256,224,0,false);snprintf(path,sizeof(path),"%s-outside-private.ppm",out);WritePpm(path,right,256*4,256,224,0,false);}
    }
    if(environment && getenv("X3_COOP_ENVIRONMENT_WEATHER_WIDE") && f==3)weather_wide_picture(left,right,outside_seat^1);
    if(environment && getenv("X3_COOP_ENVIRONMENT_MATH_POSITIVE") && (g_ppu->cgadsub&63)){
      Ppu *oracle=malloc(sizeof(*oracle));check(oracle!=NULL,"native color-math positive control allocates");*oracle=*g_ppu;PpuClearOverlayBindings(oracle);
      oracle->renderBuffer=left;oracle->renderPitch=256*4;
      for(int line=0;line<=224;++line)PpuDrawDetachedLine(oracle,line);
      oracle->cgadsub=0;oracle->renderBuffer=right;
      for(int line=0;line<=224;++line)PpuDrawDetachedLine(oracle,line);
      unsigned changed=0;for(unsigned y=0;y<224;++y)for(unsigned x=48;x<216;++x)changed+=memcmp(left+(y*256+x)*4,right+(y*256+x)*4,3)!=0;
      math_pixels+=changed;fprintf(stderr,"native ENVIRONMENT MATH VISIBLE stage%u field%u pixels%u\n",g_ram[0x1fae],f,changed);free(oracle);
    }
    size_t raster=RtlNetplaySnapshotToMemory(cut,capacity);
    uint64_t clock=g_cpu.master_cycles,instructions=cx4_instructions_executed(g_snes->cart->cx4);unsigned runs=cx4_run_ring_count(g_snes->cart->cx4);
    cx4_picture(left,0,256,false);cx4_picture(right,1,256,false);cx4_picture(split,2,256,true);
    for(unsigned y=0;y<224;++y)check(!memcmp(split+y*512*4,left+y*256*4,256*4) && !memcmp(split+(y*512+256)*4,right+y*256*4,256*4),"both actual Cx4 split fields exactly equal their selected native-owner compositions");
    if(!environment && (g_ram[0x1e99]==14 || (getenv("X3_COOP_ARENA_EDGE_REFERENCE") && X3FixedArenaFor(g_ram)))){
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
  check(cuts==4 && (environment || cx4_instructions_executed(g_snes->cart->cx4)>initial),"the actual encounter advances through all four native visual cuts");
  if(environment && getenv("X3_COOP_ENVIRONMENT_MATH_POSITIVE"))check(math_pixels>0,"actual original color math visibly changes world pixels rather than only leaving a register enabled");
  fprintf(stderr,"native CX4 PRESENTATION cuts%u fullNativeAnchorCuts%u bothViews/split exact DSPbefore%llu after%llu noRenderUpdates\n",cuts,native_cuts,(unsigned long long)initial,(unsigned long long)cx4_instructions_executed(g_snes->cart->cx4));
  if(separated){n=RtlSaveSnapshotToMemory(cut,capacity);for(unsigned f=0;f<32;++f)frame(0);size_t expected=RtlSaveSnapshotToMemory(left,capacity);check(RtlLoadSnapshotFromMemory(cut,n),"separated native weather restores its complete running world");for(unsigned f=0;f<32;++f)frame(0);size_t actual=RtlSaveSnapshotToMemory(right,capacity);same(left,expected,right,actual,"separated native weather reproduces the entire machine future");}
  if(weather_onset){
    /* Leave the region with positions/cameras only. The original controller
     * must perform its fade, graphics reset and actual retirement itself. */
    n=RtlSaveSnapshotToMemory(cut,capacity);setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);unsigned returning=outside_seat^1;
    unsigned x=setup.players[outside_seat].body[5]|(unsigned)setup.players[outside_seat].body[6]<<8,floor=944,foot=setup.players[returning].character==X3_COOP_ZERO?25:17;
    check(terrain_test_kind(x,floor)==0x3b && !terrain_test_kind(x-12,floor-1) && !terrain_test_kind(x+12,floor-1),"weather retirement returns to the actual full-foot native entry floor");
    campaign_position_only(&setup.players[returning],x,floor-foot);setup.players[returning].body[0x88]=255;
    check(X3CoopEncode(&setup,cut+contact_context_offset(cut,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(cut,n),"weather retirement changes only the returning actor position and protection");door_install_player(&setup);
    X3CoopViewState views=*X3CoopViewsState();memcpy(views.limits[returning],views.limits[outside_seat],sizeof(views.limits[returning]));memcpy(views.current[returning],views.current[outside_seat],sizeof(views.current[returning]));memcpy(views.camera[returning],views.camera[outside_seat],sizeof(views.camera[returning]));X3CoopViewsSetState(&views);vehicle_install_view(&views,setup.anchor);
    for(unsigned f=0;f<192;++f)frame(0);
    bool active=false;for(unsigned d=0x1d18;d<0x1e18;d+=16)active|=g_ram[d] && g_ram[d+10]==3;
    check(!active && !g_ram[0x1f60] && g_ppu->screenEnabled[0]==0x13 && !g_ppu->screenEnabled[1] && !g_ppu->cgwsel && !g_ppu->cgadsub && contact_hp(0) && contact_hp(1),"the original weather script actually retires and restores ordinary live gameplay after both views leave");
    for(unsigned seat=0;seat<2;++seat)weather_indoor_picture(outside_oracle,outside_reference,left,seat,star_colors,star_count,512);
  }
  free(outside_oracle);free(outside_reference);free(split);g_config.widescreen=wide;
}

/* Start from the earned corridor's actual streamed gate. This reuses the
 * qualified route; it neither repeats the frozen later navigation nor
 * supplies a door phase, room/cache index or completion flag. */
static void native_door_presentation_checks(const char *fixture,uint8_t *base,uint8_t *left,uint8_t *right,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==10 && !X3CoopRuntimeSceneActive(),"door picture starts at the actual earned first fortress gate approach");
  unsigned leader=getenv("X3_COOP_DOOR_VISUAL_LEADER")?(unsigned)atoi(getenv("X3_COOP_DOOR_VISUAL_LEADER")):1;check(leader<2,"door picture names a real leader");
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);
  if(getenv("X3_COOP_DOOR_VISUAL_REVERSED")){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}
  setup.current=setup.anchor=(uint8_t)(leader^1);for(unsigned seat=0;seat<2;++seat)setup.players[seat].body[0x88]=255;
  check(X3CoopEncode(&setup,base+contact_context_offset(base,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,n),"door picture selects only roster, prior owner and personal protection");door_install_player(&setup);X3CoopViewsConfigure(true);
  unsigned door=0;for(unsigned d=0x1518;d<0x1818;d+=48)if(g_ram[d] && g_ram[d+10]==7 && view_read(d+12)==0xfad9)door=d;
  check(door && view_read(door+5)==2552 && contact_hp(0) && contact_hp(1),"actual corridor already contains its streamed native gate and living pair");
  unsigned start_x=view_read(0x1e5d),pan_cuts=0,post_cuts=0,exit_field=0;bool entered=false,exited=false;uint8_t *split=malloc(512u*224u*4u);check(split!=NULL,"door split picture allocates");
  for(unsigned f=0;f<3600;++f){unsigned keys=exited?0:SNES_PAD_RIGHT;
    if(!exited && f%120<6 && (g_ram[0x1f3f]&128))keys|=SNES_PAD_A;
    frame((uint32_t)keys<<(leader*12));
    bool active=X3CoopRuntimeSceneActive();entered|=active;
    if(entered && !active && !exited && support_position(0,5)>2584 && support_position(1,5)>2584){exited=true;exit_field=f;}
    bool pan=active && pan_cuts<4 && view_read(0x1e5d)>=start_x+(pan_cuts+1)*64;
    unsigned age=f-exit_field;bool post=exited && (age==0 || age==8 || age==31 || age==63);
    if(!pan && !post)continue;
    check(contact_hp(0) && contact_hp(1) && !X3CoopRuntimeState()->wipe,"actual native door pan retains both living participants");
    n=RtlSaveSnapshotToMemory(base,capacity);char tag[64];snprintf(tag,sizeof(tag),"door-%s%u",pan?"pan":"post",pan?pan_cuts:post_cuts);
    if(pan && !pan_cuts){for(unsigned i=0;i<32;++i)frame(0);size_t expected=RtlSaveSnapshotToMemory(left,capacity);check(RtlLoadSnapshotFromMemory(base,n),"actual native door pan restores before replay");for(unsigned i=0;i<32;++i)frame(0);size_t actual=RtlSaveSnapshotToMemory(right,capacity);same(left,expected,right,actual,"the actual native camera pan repeats its entire machine future");check(RtlLoadSnapshotFromMemory(base,n),"door picture restores the actual sampled pan field");}
    native_display_checks(tag,base,n,left,right,capacity);
    size_t canonical=RtlNetplaySnapshotToMemory(base,capacity);uint64_t clock=g_cpu.master_cycles;
    cx4_picture(left,0,256,false);cx4_picture(right,1,256,false);cx4_picture(split,2,256,true);
    for(unsigned y=0;y<224;++y)check(!memcmp(split+y*512*4,left+y*256*4,256*4) && !memcmp(split+(y*512+256)*4,right+y*256*4,256*4),"actual door pan Split output exactly matches each selected original-owner view");
    const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");if(out && *out){char path[1024];snprintf(path,sizeof(path),"%s-%s-split.ppm",out,tag);WritePpm(path,split,512*4,512,224,0,false);}
    size_t untouched=RtlNetplaySnapshotToMemory(right,capacity);same(base,RtlNetplaySnapshotHashedBytes(canonical),right,RtlNetplaySnapshotHashedBytes(untouched),"door view composition leaves the complete canonical state unchanged");check(clock==g_cpu.master_cycles,"door view composition advances no guest time");
    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
    if(pan)++pan_cuts;else ++post_cuts;if(post_cuts==4)break;
  }
  fprintf(stderr,"native DOOR PICTURE leader%u panCuts%u postCuts%u camera%u->%u actors%u,%u/%u,%u\n",leader,pan_cuts,post_cuts,start_x,view_read(0x1e5d),support_position(0,5),support_position(0,8),support_position(1,5),support_position(1,8));
  if(!exited || pan_cuts!=4 || post_cuts!=4)check(RtlSaveSnapshot("native-door-picture-unqualified.sav"),"unfinished actual door picture retains its complete machine");
  check(entered && exited && pan_cuts==4 && post_cuts==4 && !X3CoopRuntimeSceneActive() && !campaign_body(0)[0x6e] && !campaign_body(1)[0x6e],"actual native door opening, complete camera pan, closure and free two-player views all qualify");free(split);
}

/* Reuse the earned Hornet mark/flight, original PPU oracle and pure view
 * composition. No attachment, native priority or retirement result is set. */
static void native_attachment_presentation_checks(const char *fixture,uint8_t *cut,uint8_t *left,uint8_t *right,size_t capacity){
  check(RtlLoadSnapshot(fixture) && g_ram[0x1fae]==1,"attachment picture loads its actually earned Hornet mark and homing flight");
  unsigned marker=0;for(unsigned d=0x1318;d<0x1518;d+=64)if(g_ram[d] && g_ram[d+10]==0x21 && g_ram[d+0x37]==4)marker=d;
  check(marker && X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual attachment picture retains both living participants and a native marker");
  X3CoopViewsConfigure(true);bool wide=g_config.widescreen;g_config.widescreen=false;
  X3Display_SetCoopPanelWidth(256);X3Display_SetPresentationSeat(-1);X3Display_SetLocalSplit(false);
  uint8_t *split=malloc(512u*224u*4u);check(split!=NULL,"attachment Split comparison allocates bounded pixels");
  unsigned cuts=0,positive=0,personal_colors=0,tick=g_ram[0x9cc];
  for(unsigned f=0;f<128;++f){
    frame(0);if(f!=3 && f!=15 && f!=33 && f!=63)continue;
    size_t n=RtlSaveSnapshotToMemory(cut,capacity);char tag[64];snprintf(tag,sizeof(tag),"attachment-field%u",f);
    native_display_checks(tag,cut,n,left,right,capacity);
    n=RtlNetplaySnapshotToMemory(cut,capacity);
    cx4_picture(left,0,256,false);cx4_picture(right,1,256,false);cx4_picture(split,2,256,true);
    for(unsigned y=0;y<224;++y)check(!memcmp(split+y*512*4,left+y*256*4,256*4) && !memcmp(split+(y*512+256)*4,right+y*256*4,256*4),"both attachment Split halves retain their own complete selected-view pixels");
    if(!personal_colors){const X3CoopPresentedFrame *shown=X3CoopPresentationVisible();unsigned anchor=shown->players.anchor;
      for(unsigned d=0;d<shown->count;++d){X3CoopWorldDraw *draw=(X3CoopWorldDraw *)&shown->actors[d];
        if(!draw->owner || draw->owner==anchor+1 || draw->address==X3_COOP_BODY || ((draw->body[0x11]>>1)&7)>=2)continue;
        uint8_t owner=draw->owner;cx4_picture(left,anchor,256,false);draw->owner=(uint8_t)(anchor+1);cx4_picture(right,anchor,256,false);draw->owner=owner;
        for(unsigned pixel=0;pixel<256*224;++pixel)personal_colors+=memcmp(left+pixel*4,right+pixel*4,3)!=0;
        cx4_picture(right,anchor,256,false);check(!memcmp(left,right,256u*224u*4u),"restoring the creator restores every owned effect color pixel");
        if(personal_colors){cx4_picture(split,2,256,false);check(X3Display_RenderedPrivateFrame(),"actual narrow Shared uses creator colors for the partner's native personal-ramp effect");
          for(unsigned y=0;y<224;++y)check(!memcmp(split+(y*256+40)*4,left+(y*256+40)*4,192*4),"Shared world and owned effect colors match the correct selected native-owner field");break;
        }
      }
    }
    if(!positive){
      const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
      /* The latched kind4 controller can be hidden; its real visible reticle
       * is the owned native0B/group60 effect emitted around the marked body. */
      for(unsigned d=0;d<visible->count;++d)if(visible->actors[d].address==marker ||
          (visible->actors[d].owner && visible->actors[d].body[10]==0x0b && visible->actors[d].body[0x16]==0x60)){
        X3CoopWorldDraw *draw=(X3CoopWorldDraw *)&visible->actors[d];uint8_t x[2]={draw->body[5],draw->body[6]};
        unsigned anchor=X3CoopRuntimeState()->anchor;cx4_picture(left,anchor,256,false);
        unsigned shifted=(draw->body[5]|draw->body[6]<<8)+4096;draw->body[5]=(uint8_t)shifted;draw->body[6]=(uint8_t)(shifted>>8);
        cx4_picture(right,anchor,256,false);memcpy(draw->body+5,x,2);
        for(unsigned pixel=0;pixel<256*224;++pixel)positive+=memcmp(left+pixel*4,right+pixel*4,3)!=0;
        cx4_picture(right,anchor,256,false);check(!memcmp(left,right,256u*224u*4u),"restoring the actual attachment coordinate restores every composed pixel");break;
      }
    }
    size_t after=RtlNetplaySnapshotToMemory(right,capacity);
    same(cut,RtlNetplaySnapshotHashedBytes(n),right,RtlNetplaySnapshotHashedBytes(after),"attachment priority/view controls leave the entire canonical simulation unchanged");
    X3Display_SetPresentationSeat(-1);X3Display_SetLocalSplit(false);++cuts;
  }
  fprintf(stderr,"native ATTACHMENT PRESENTATION cuts%u marker%04x positivePixels%u personalColorPixels%u nativePPU/priority exact bothViews/Split/Shared exact canonicalUnchanged\n",cuts,marker,positive,personal_colors);
  check(cuts==4 && positive && personal_colors && (uint8_t)(g_ram[0x9cc]-tick)==128,"actual nonempty attachment, creator colors and native priority execute128 shared fields across four visual cuts");
  free(split);g_config.widescreen=wide;
}
