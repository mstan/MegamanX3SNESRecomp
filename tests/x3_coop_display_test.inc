#include "../src/x3_display.h"
#include "../src/x3_native_resources.h"
#include "../src/x3_coop_hud.h"
static void native_resource_checks(uint8_t *original,size_t size,uint8_t *bytes,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"native resource fixture restores");
  const unsigned keys[]={0,2,40};
  for(unsigned n=0;n<3;++n){
    unsigned key=keys[n];size_t length=0;
    check(X3ResourceDecode(g_snes->cart->rom,g_snes->cart->romImageSize,key,bytes,capacity,&length),
          "native compressed stage resource decodes");
    unsigned cache=g_ram[0x18000+key*2]|g_ram[0x18001+key*2]<<8;
    check(cache+length<=32768 && !memcmp(bytes,g_ram+0x10000+cache,length),
          "read-only resource decoder matches the actual native decompressed cache");
    if(key==2){
      X3ResourceSegment segments[X3_RESOURCE_SEGMENTS];unsigned count=0;
      check(X3ResourceLayout(g_snes->cart->rom,g_snes->cart->romImageSize,key,0x1000,segments,&count) && count,
            "native enemy file uses its original upload segmentation");
      bool equal=true;
      for(unsigned j=0;j<count;++j){const X3ResourceSegment *s=&segments[j];
        check(s->source+s->length<=length,"native segment fits its decompressed file");
        for(unsigned i=0;i<s->length;++i)
          if(bytes[s->source+i]!=(uint8_t)(g_ppu->vram[s->destination+i/2]>>((i&1)*8)))equal=false;
      }
      check(equal,"read-only native upload layout matches actual enemy VRAM bytes");
      uint16_t colors[256]={0};
      check(X3ResourcePalette(g_snes->cart->rom,g_snes->cart->romImageSize,8,64,colors),"native enemy palette list resolves");
      check(!memcmp(colors+0xc0,g_ppu->cgram+0xc0,32),"read-only resource palette matches native enemy CGRAM");
    }
  }
}
static void remote_resource_checks(uint8_t *original,size_t size,uint8_t *work,size_t capacity){
  check(RtlLoadSnapshotFromMemory(original,size),"remote resource setup restores");
  X3CoopState setup=*X3CoopRuntimeState();
  setup.players[1].body[5]=0x64;setup.players[1].body[6]=5; /*1380*/
  setup.players[1].body[8]=0x70;setup.players[1].body[9]=2; /*624*/
  memset(setup.players[1].body+0x1a,0,6);
  memcpy(work,original,size);size_t offset=contact_context_offset(original,size);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,size),
        "remote room teleport precondition restores");
  X3CoopViewsConfigure(true);
  X3CoopViewState views={0};views.independent=views.initialized=1;views.stage=1;
  views.valid=views.camera_valid=3;
  views.camera[0][0]=256;views.camera[0][1]=1024;
  views.camera[1][0]=1280;views.camera[1][1]=512;
  for(unsigned seat=0;seat<2;++seat){
    views.limits[seat][0]=views.current[seat][0]=seat?1280:256;
    views.limits[seat][1]=views.current[seat][1]=seat?1536:1024;
    views.limits[seat][2]=views.current[seat][2]=seat?512:1024;
    views.limits[seat][3]=views.current[seat][3]=seat?512:1024;
  }
  /* The teleport/camera setup bypasses route/camera qualification. Original
   * union admission and the actual enemy constructor still decide the result. */
  X3CoopViewsSetState(&views);CpuState resume=g_cpu;
  view_native_call(0x00de3a,0,false);
  g_cpu.A=resume.A;g_cpu.X=resume.X;g_cpu.Y=resume.Y;g_cpu.D=resume.D;
  g_cpu.DB=resume.DB;g_cpu.PB=resume.PB;g_cpu.P=resume.P;cpu_p_to_mirrors(&g_cpu);
  for(unsigned i=0;i<3;++i)frame(0);
  /* Class2F is the authored Ride Armor station: initialization relocates it
   * from event1424,592 to its stage-table position1712,432. Move the QA actor
   * into that actual upper room, using its native E23A limits. */
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity);
  setup=*X3CoopRuntimeState();
  setup.players[1].body[5]=0x7c;setup.players[1].body[6]=6; /*1660*/
  setup.players[1].body[8]=0xd0;setup.players[1].body[9]=1; /*464*/
  memset(setup.players[1].body+0x1a,0,6);
  check(X3CoopEncode(&setup,work+offset,X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,prepared),
        "native Ride Armor station room teleport restores");
  views=*X3CoopViewsState();views.camera[1][0]=1532;views.camera[1][1]=304;
  views.limits[1][1]=views.current[1][1]=2048;views.limits[1][2]=views.current[1][2]=256;
  X3CoopViewsSetState(&views);
  for(unsigned i=0;i<3;++i)frame(0);
  bool found=false;
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  for(unsigned i=0;i<visible->count;++i){const X3CoopWorldDraw *d=&visible->actors[i];
    if(d->address>=0xd18 && d->address<0x10d8 && d->body[10]==0x2f && d->body[0x16]==0x3f)found=true;
  }
  if(!found){
    fprintf(stderr,"remote resource: draws=%u camera=%u,%u p2=%u,%u phase=%u\n",visible->count,
        visible->views[1][0],visible->views[1][1],coop_x(1),X3CoopRuntimeState()->players[1].body[8]|X3CoopRuntimeState()->players[1].body[9]<<8,g_ram[0x1f18]);
    for(unsigned d=0xd18;d<0x10d8;d+=64)if(g_ram[d])
      fprintf(stderr,"remote enemy %04x class=%02x state=%02x/%02x group=%02x xy=%u,%u visible=%02x\n",d,g_ram[d+10],g_ram[d+1],g_ram[d+2],g_ram[d+0x16],g_ram[d+5]|g_ram[d+6]<<8,g_ram[d+8]|g_ram[d+9]<<8,g_ram[d+14]);
  }
  check(found,"actual stage1 Ride Armor station submits native group3F in P2's remote room");
  check(g_ram[0x1f18]==0 && !g_ram[0x1823c] && !g_ram[0x1833c],
        "remote actor is visible before the anchor loads file3C's placement and palette");
  memcpy(work,g_ram,0x20000);uint64_t clock=g_cpu.master_cycles;
  X3Display_SetPresentationSeat(1);CaptureSimulationFrame(1);
  check(X3Display_RenderedPrivateFrame(),"remote resource screenshot uses actual private scanout");
  check(clock==g_cpu.master_cycles && !memcmp(work,g_ram,0x20000),
        "resolving and painting remote resources does not mutate the CPU or shared world");
  const char *output=getenv("X3_COOP_REMOTE_RESOURCE_OUTPUT");
  if(output && *output)check(RtlSaveSnapshot(output),"remote resource visual fixture saves");
  const char *image=getenv("X3_COOP_DISPLAY_OUTPUT");
  if(image && *image){char path[1024];snprintf(path,sizeof(path),"%s-remote.ppm",image);
    WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);}
  X3Display_SetPresentationSeat(-1);
  check(RtlLoadSnapshotFromMemory(original,size),"remote resource probes restore");(void)capacity;
}
static void native_display_checks(const char *tag,uint8_t *original,size_t size,uint8_t *native,uint8_t *private,size_t capacity){
  /* Compare the original native HUD with the private owning meter. Shared
   * two-player layouts have separate composition/ownership checks. */
  unsigned hud=X3CoopHudLayout();X3CoopHudSetLayout(X3_COOP_HUD_NATIVE_REFERENCE);
  bool wide=g_config.widescreen;g_config.widescreen=false;
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
  frame(0);frame(0);
  check(g_ppu->renderPitch==256*4,"native display comparison uses an actual256px field");
  if(X3Display_RenderedPrivateFrame() || getenv("X3_COOP_ATTACHMENT_VISUAL_STATE")){
    /* Shared may now require creator colors. Render the original PPU/OAM
     * independently so the reference can never become a private self-check. */
    Ppu *oracle=malloc(sizeof(*oracle));check(oracle!=NULL,"native ordering reference owns a detached original PPU");*oracle=*g_ppu;
    oracle->renderBuffer=native;oracle->renderPitch=256*4;
    for(int line=0;line<=224;++line)PpuDrawDetachedLine(oracle,line);free(oracle);
  }else memcpy(native,g_ppu->renderBuffer,256u*224u*4u);
  uint8_t owners[X3_COOP_WORLD_DRAWS];X3CoopPresentedFrame *shown=(X3CoopPresentedFrame *)X3CoopPresentationVisible();
  /* The native common ramps cannot express two characters' FX colors at
   * once. Match that source only for the ordering oracle; actual creator
   * colors have independent original-character and visible-positive checks. */
  for(unsigned d=0;d<shown->count;++d){owners[d]=shown->actors[d].owner;
    if(getenv("X3_COOP_ATTACHMENT_VISUAL_STATE") && shown->actors[d].address!=X3_COOP_BODY)shown->actors[d].owner=0;
  }
  X3CoopViewsConfigure(true);X3Display_SetPresentationSeat(X3CoopRuntimeState()->anchor);
  check(X3Display_BeginIndependentFrame(g_ppu),"private primary compositor is active for the comparison");
  g_rtl_game_info->draw_ppu_frame();
  check(X3Display_RenderedPrivateFrame(),"actual render call uses the private compositor rather than its native fallback");
  memcpy(private,g_ppu->renderBuffer,256u*224u*4u);
  for(unsigned d=0;d<shown->count;++d)shown->actors[d].owner=owners[d];
  unsigned differences=0,nonblack=0;int left=256,right=0,top=224,bottom=0;
  for(unsigned y=0;y<224;++y)for(unsigned x=0;x<256;++x){
    unsigned at=(y*256+x)*4;
    if(native[at] || native[at+1] || native[at+2])++nonblack;
    /* Personal badges and charge pips are an approved co-op HUD feature;
     * the original single-player raster cannot be their visual oracle. The
     * actual health/energy strips and every world pixel remain strict. */
    bool own_badge=(getenv("X3_COOP_CX4_VISUAL_STATE") || getenv("X3_COOP_ATTACHMENT_VISUAL_STATE") || getenv("X3_COOP_DOOR_VISUAL_STATE")) && x>=8 && x<40 && y>=80 && y<96;
    if(!own_badge && memcmp(native+at,private+at,3)){
      ++differences;if((int)x<left)left=x;if((int)x>right)right=x;if((int)y<top)top=y;if((int)y>bottom)bottom=y;
    }
  }
  fprintf(stderr,"private/native display comparison %s: differences=%u nonblack=%u bounds=%d,%d..%d,%d\n",tag,differences,nonblack,left,top,right,bottom);
  const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");
  if(out && *out){char path[1024];
    snprintf(path,sizeof(path),"%s-%s-native.ppm",out,tag);WritePpm(path,native,256*4,256,224,0,false);
    snprintf(path,sizeof(path),"%s-%s-private.ppm",out,tag);WritePpm(path,private,256*4,256,224,0,false);
  }
  check(nonblack>1000,"display comparison contains actual native stage content");
  if(differences && getenv("X3_COOP_ATTACHMENT_VISUAL_STATE"))check(RtlSaveSnapshot("attachment-priority-unqualified.sav"),"actual native attachment priority mismatch retains its completed OAM and presentation metadata");
  if(differences && getenv("X3_COOP_ENVIRONMENT_VISUAL")){
    check(RtlSaveSnapshot("native-environment-picture-unqualified.sav"),"actual environment mismatch retains its complete machine");
    const X3CoopPresentedFrame *v=X3CoopPresentationVisible();
    for(unsigned i=0;i<v->count;++i){const X3CoopWorldDraw *d=&v->actors[i];const uint8_t *b=d->body;
      int x=(int)(b[5]|(unsigned)b[6]<<8)-v->camera[0],y=(int)(b[8]|(unsigned)b[9]<<8)-v->camera[1];
      if(x>=left-64 && x<=right+64 && y>=top-64 && y<=bottom+64)fprintf(stderr,"native ENV PIXEL ACTOR address%04x owner%u class%02x group%02x pose%02x flags%02x tile%02x XY%d,%d\n",d->address,d->owner,b[10],b[22],b[23],b[17],b[24],x,y);
    }
  }
  check(!differences,"private primary view matches the native world, body and health HUD pixels");
  X3Display_SetPresentationSeat(-1);g_config.widescreen=wide;X3CoopHudSetLayout(hud);
  check(RtlLoadSnapshotFromMemory(original,size),"display probes restore their starting machine");
  (void)capacity;
}
static void independent_display_checks(uint8_t *original,size_t size,uint8_t *left,uint8_t *right,size_t capacity){
  bool wide=g_config.widescreen;g_config.widescreen=false;
  uint8_t *world=malloc(0x20000),*split=malloc(512u*224u*4u);
  check(world && split,"split display comparison buffers allocate");memcpy(world,g_ram,0x20000);
  uint64_t clock=g_cpu.master_cycles;
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(0);CaptureSimulationFrame(1);
  check(X3Display_RenderedPrivateFrame(),"separated P1 display uses its private renderer");
  memcpy(left,g_ppu->renderBuffer,256u*224u*4u);
  X3Display_SetPresentationSeat(1);g_rtl_game_info->draw_ppu_frame();
  check(X3Display_RenderedPrivateFrame(),"separated P2 display uses its private renderer");
  memcpy(right,g_ppu->renderBuffer,256u*224u*4u);
  check(memcmp(left,right,256u*224u*4u)!=0,"separated players render distinct authored world regions");
  X3Display_SetLocalSplit(true);g_rtl_game_info->draw_ppu_frame();
  check(X3Display_RenderedPrivateFrame() && X3Display_DrawCoopFrame(split,512*4,NULL,512,224,1),
        "split display composes both independently rendered fields");
  bool equal=true;
  for(unsigned y=0;y<224;++y)
    if(memcmp(split+y*512*4,left+y*256*4,256*4) || memcmp(split+(y*512+256)*4,right+y*256*4,256*4))equal=false;
  check(equal,"split fields exactly match each player's selected independent view");
  check(g_cpu.master_cycles==clock && !memcmp(world,g_ram,0x20000),
        "selected and split rendering do not advance the CPU or mutate shared world RAM");
  const char *out=getenv("X3_COOP_DISPLAY_OUTPUT");
  if(out && *out){char path[1024];
    snprintf(path,sizeof(path),"%s-p1.ppm",out);WritePpm(path,left,256*4,256,224,0,false);
    snprintf(path,sizeof(path),"%s-p2.ppm",out);WritePpm(path,right,256*4,256,224,0,false);
    snprintf(path,sizeof(path),"%s-split.ppm",out);WritePpm(path,split,512*4,512,224,0,false);
  }
  check(capacity>=896u*224u*4u,"the adaptive comparison has room for both wider fields");
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(0);
  X3Display_SetCoopPanelWidth(342);g_rtl_game_info->draw_ppu_frame();
  check(X3Display_DrawCoopFrame(right,342*4,NULL,342,224,1),"adaptive selected view presents the full342px field");
  equal=true;unsigned detail[2]={0,0};
  for(unsigned y=0;y<224;++y){
    const uint8_t *row=right+y*342*4;
    if(memcmp(row+(43+48)*4,left+(y*256+48)*4,168*4))equal=false;
    for(unsigned x=1;x<43;++x){
      if(memcmp(row+x*4,row+(x-1)*4,3))++detail[0];
      if(memcmp(row+(299+x)*4,row+(298+x)*4,3))++detail[1];
    }
  }
  check(equal && detail[0]>100 && detail[1]>100,
        "the wider private field preserves central world pixels and resolves real detail in both margins");
  check(RtlLoadSnapshotFromMemory(original,size),"paused wide restoration starts from the actual machine");
  check(X3Display_DrawCoopFrame(right,342*4,left,342,224,1),"paused restore presents its native field before the new wide raster");
  equal=true;
  for(unsigned y=0;y<224;++y){
    const uint8_t *row=right+y*342*4;
    if(memcmp(row+43*4,left+y*256*4,256*4) || row[0] || row[1] || row[2])equal=false;
  }
  check(equal,"paused restore discards the old wide scene and centers the current native picture");
  X3Display_SetLocalSplit(true);X3Display_SetCoopPanelWidth(448);
  g_rtl_game_info->draw_ppu_frame();
  check(X3Display_DrawCoopFrame(right,896*4,NULL,896,224,1),"ultrawide split presents two real448px fields");
  unsigned distinct=0;for(unsigned y=0;y<224;++y)
    distinct+=memcmp(right+y*896*4,right+(y*896+448)*4,448*4)!=0;
  check(distinct>100 && g_ppu->extraLeftRight==0 && g_cpu.master_cycles==clock && !memcmp(world,g_ram,0x20000),
        "wider split shows both distinct regions without changing native PPU width, CPU or world RAM");
  size_t baseline=0;
  for(int mode=-1;mode<5;++mode){
    check(RtlLoadSnapshotFromMemory(original,size),"display-mode simulation comparison starts from the same machine");
    X3Display_SetLocalSplit(mode==2 || mode==4);X3Display_SetPresentationSeat(mode==3?0:mode>=0 && mode<2?mode:-1);
    X3Display_SetCoopPanelWidth(mode==3?342:mode==4?448:256);
    for(unsigned i=0;i<10;++i)frame(0);
    if(mode==-1)baseline=RtlSaveSnapshotToMemory(left,capacity);
    else {size_t candidate=RtlSaveSnapshotToMemory(right,capacity);
      same(left,baseline,right,candidate,"local presentation choice preserves the complete simulation snapshot");}
  }
  free(world);free(split);X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);g_config.widescreen=wide;
  X3Display_SetCoopPanelWidth(256);
  check(RtlLoadSnapshotFromMemory(original,size),"split display probes restore their starting machine");(void)capacity;
}

/* Native unused OAM has Y>=224 in the game's8/16px object mode. Give those
 * real hidden slots bright valid tiles and both X-sign encodings; they must
 * remain absent from native, selected, split and wider output. Restore OAM
 * before the next genuine input/world field. */
static void hidden_oam_picture(uint8_t *pixels,unsigned seat,unsigned panel,bool split){
  X3Display_SetPresentationSeat(seat<2?(int)seat:-1);X3Display_SetLocalSplit(split);X3Display_SetCoopPanelWidth(panel);
  g_rtl_game_info->draw_ppu_frame();unsigned width=panel*(split?2:1);
  if(split || panel>256)check(X3Display_DrawCoopFrame(pixels,width*4,NULL,width,224,1),"hidden-OAM probe presents the actual complete co-op field");
  else{check(g_ppu->renderPitch==256*4,"hidden-OAM native/selected field is256pixels");memcpy(pixels,g_ppu->renderBuffer,256u*224u*4u);}
}
static void native_hidden_oam_checks(const char *normal,const char *reversed,uint8_t *reference,uint8_t *changed){
  bool wide=g_config.widescreen;g_config.widescreen=false;
  static const unsigned seats[]={2,0,1,2,2,0,1,2},panels[]={256,256,256,256,342,342,342,448};
  unsigned fields=0,draws=0,minimum=128;
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal),"hidden-OAM probe loads its actual native-input two-region roster");X3CoopViewsConfigure(true);X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3Display_SetCoopPanelWidth(256);
    for(unsigned f=0;f<32;++f){
      unsigned tick=g_ram[0x9cc];frame(0);check((uint8_t)(g_ram[0x9cc]-tick)==1,"hidden-OAM qualification advances exactly one actual world field");++fields;
      check(PPU_objSize(g_ppu)==0,"actual X3 OAM uses the qualified8/16pixel object sizes");
      uint16_t oam[256];uint8_t high[32];memcpy(oam,g_ppu->oam,sizeof(oam));memcpy(high,g_ppu->highOam,sizeof(high));
      unsigned hidden=0;for(unsigned i=23;i<128;++i)hidden+=(oam[2*i]>>8)>=224 && (oam[2*i]>>8)<=240;
      if(hidden<minimum)minimum=hidden;check(hidden>=32,"the actual submitted native packet contains many genuine hidden entries");
      /* Select a real nonempty native character tile; a wrap would be visible. */
      unsigned tile=0;for(unsigned i=23;i<128;++i)if((oam[2*i]>>8)<224){tile=oam[2*i+1];break;}
      check(tile!=0,"hidden-OAM negative control uses an actual visible native sprite tile");
      tile=(tile&~0x3000u)|0x3000u; /* Same visible highest-priority marker in positive/hidden cases. */
      for(unsigned mode=0;mode<8;++mode){bool split=mode==3 || mode==7;unsigned width=panels[mode]*(split?2:1);
        hidden_oam_picture(reference,seats[mode],panels[mode],split);
        if(!mode && !f){unsigned slot=23;while(slot<128 && ((oam[2*slot]>>8)<224 || (oam[2*slot]>>8)>240))++slot;
          check(slot<128 && !X3Display_RenderedPrivateFrame(),"the hidden tile positive control uses the actual native OAM renderer");
          g_ppu->oam[2*slot]=0x7060;g_ppu->oam[2*slot+1]=(uint16_t)((tile&~0x3000u)|0x3000);unsigned shift=(slot&3)*2;
          g_ppu->highOam[slot>>2]=(uint8_t)((g_ppu->highOam[slot>>2]&~(3u<<shift))|(2u<<shift));
          hidden_oam_picture(changed,2,256,false);check(memcmp(reference,changed,256u*224u*4u)!=0,"the same real tile actually becomes visible when its native Y is moved onscreen");
          memcpy(g_ppu->oam,oam,sizeof(oam));memcpy(g_ppu->highOam,high,sizeof(high));hidden_oam_picture(reference,seats[mode],panels[mode],split);
        }
        for(unsigned sign=0;sign<2;++sign){
          for(unsigned i=23;i<128;++i)if((oam[2*i]>>8)>=224 && (oam[2*i]>>8)<=240){
            g_ppu->oam[2*i]=(uint16_t)((oam[2*i]&0xff00)|((i*47+13)&255));g_ppu->oam[2*i+1]=(uint16_t)tile;
            unsigned shift=(i&3)*2;g_ppu->highOam[i>>2]=(uint8_t)((g_ppu->highOam[i>>2]&~(3u<<shift))|((2u|sign)<<shift));
          }
          hidden_oam_picture(changed,seats[mode],panels[mode],split);
          same(reference,(size_t)width*224*4,changed,(size_t)width*224*4,"bright real hidden OAM never wraps or flashes into either view or its margin");++draws;
          memcpy(g_ppu->oam,oam,sizeof(oam));memcpy(g_ppu->highOam,high,sizeof(high));
        }
      }
      X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3Display_SetCoopPanelWidth(256);
      check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"actual hidden-OAM fields preserve both participants");
    }
  }
  fprintf(stderr,"native HIDDEN OAM fields%u comparisons%u minimumHidden%u positiveControls2 native256 selected256/342 Shared256/342 Split512/896 bothXsigns exactPixels\n",fields,draws,minimum);
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);X3Display_SetCoopPanelWidth(256);g_config.widescreen=wide;
}
