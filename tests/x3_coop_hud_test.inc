static void native_hud_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint16_t xhud[X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT],zhud[X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT];
  uint16_t before_x[sizeof(xhud)/2],before_z[sizeof(zhud)/2];
  uint8_t *equipped=malloc(capacity),*image=malloc(256u*224u*4u);
  check(equipped && image,"HUD reference buffers allocate");
  for(unsigned reversed=0;reversed<2;++reversed){
    if(reversed){const char *fixture=getenv("X3_COOP_HUD_REVERSED_STATE");
      check(fixture && *fixture && RtlLoadSnapshot(fixture),"HUD reversed roster uses its real native enrollment and inherited CHR");
    }else check(RtlLoadSnapshotFromMemory(original,size),"HUD ownership starts from native stage1");
    X3CoopState setup=*X3CoopRuntimeState();
    unsigned x=setup.players[0].character==X3_COOP_X?0:1,zero=x^1;
    setup.current=setup.anchor=(uint8_t)x;
    setup.players[x].maximum_health=32;setup.players[x].body[0x27]=19;
    setup.players[zero].body[0x27]=13;
    size_t source=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,source),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,source),"unequal HUD health/roster precondition restores");
    door_install_player(&setup);g_ram[0x1fbc]=0x48; /* Acquired weapon/8 energy prerequisite. */
    frame(SNES_PAD_R<<(x*12));for(unsigned i=0;i<4;++i)frame(0);
    check(X3CoopRuntimeState()->players[x].body[0x33]==2,"actual native weapon-cycle input selects the acquired weapon");
    size_t n=RtlSaveSnapshotToMemory(equipped,capacity);
    native_display_checks("weapon-hud",equipped,n,work,expected,capacity);
    const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
    X3CoopHudPlayer(g_ppu,visible,x,g_snes->cart->rom,g_snes->cart->romImageSize,false,xhud);
    X3CoopHudPlayer(g_ppu,visible,zero,g_snes->cart->rom,g_snes->cart->romImageSize,false,zhud);
    unsigned weapon_pixels=0;bool zero_clear=true;
    for(unsigned y=0;y<X3_COOP_HUD_HEIGHT;++y)for(unsigned col=16;col<32;++col){
      weapon_pixels+=xhud[y*32+col]!=0;
      zero_clear=zero_clear && !zhud[y*32+col];
    }
    check(zero_clear,"Zero has no invented special-weapon meter");
    check(weapon_pixels>100,"X's private HUD contains the actual native acquired-weapon meter/icon");
    memcpy(before_x,xhud,sizeof(xhud));memcpy(before_z,zhud,sizeof(zhud));
    uint8_t energy=g_ram[0x1fbc];uint16_t color=g_ppu->cgram[0xb1];
    g_ram[0x1fbc]=0x41;g_ppu->cgram[0xb1]^=32767;
    X3CoopHudPlayer(g_ppu,visible,x,g_snes->cart->rom,g_snes->cart->romImageSize,false,xhud);
    same(before_x,sizeof(before_x),xhud,sizeof(xhud),"displayed HUD retains submitted energy and colors through live changes");
    g_ram[0x1fbc]=energy;g_ppu->cgram[0xb1]=color;
    for(unsigned i=0;i<180;++i)frame(SNES_PAD_Y<<(zero*12));
    visible=X3CoopPresentationVisible();
    check(visible->players.players[zero].body[0x58]>=4,"actual native Zero charge publishes its owning HUD level");
    X3CoopHudPlayer(g_ppu,visible,x,g_snes->cart->rom,g_snes->cart->romImageSize,false,xhud);
    X3CoopHudPlayer(g_ppu,visible,zero,g_snes->cart->rom,g_snes->cart->romImageSize,false,zhud);
    same(before_x,sizeof(before_x),xhud,sizeof(xhud),"Zero charging cannot alter X's health, weapon or charge HUD");
    check(memcmp(before_z,zhud,sizeof(zhud))!=0,"Zero's own HUD reflects its actual native charge");
    memcpy(before_z,zhud,sizeof(zhud));n=RtlSaveSnapshotToMemory(equipped,capacity);
    for(unsigned i=0;i<30;++i)frame(0);
    check(RtlLoadSnapshotFromMemory(equipped,n),"charged HUD snapshot restores");
    X3CoopHudPlayer(g_ppu,X3CoopPresentationVisible(),zero,g_snes->cart->rom,g_snes->cart->romImageSize,false,zhud);
    same(before_z,sizeof(before_z),zhud,sizeof(zhud),"current-build restore preserves visible charge, palette and meter pixels");

    X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
    bool wide=g_config.widescreen;g_config.widescreen=false;
    size_t reference=0;
    const unsigned modes[]={X3_COOP_HUD_NATIVE_REFERENCE,X3_COOP_HUD_COMPACT,X3_COOP_HUD_HORIZONTAL};
    uint32_t render_flags=g_ppu->renderFlags;
    for(unsigned renderer=0;renderer<2;++renderer){
    fprintf(stderr,"shared HUD renderer=%s roster%u\n",renderer?"new":"legacy",reversed);
    for(unsigned m=0;m<3;++m){
      check(RtlLoadSnapshotFromMemory(equipped,n),"shared HUD comparison restores the same machine");
      g_ppu->renderFlags=(render_flags&~kPpuRenderFlags_NewRenderer)|(renderer?kPpuRenderFlags_NewRenderer:0);
      X3CoopHudSetLayout(modes[m]);
      PpuOverlayCapture capture=g_ppu->overlayCaptures[kPpuOverlaySource_Obj];
      uint8_t *surface=g_ppu->overlayRenderBuffer[kPpuOverlaySource_Obj];
      uint32_t pitch=g_ppu->overlayRenderPitch[kPpuOverlaySource_Obj];
      g_rtl_game_info->draw_ppu_frame();
      check(!memcmp(&capture,&g_ppu->overlayCaptures[kPpuOverlaySource_Obj],sizeof(capture)) &&
            surface==g_ppu->overlayRenderBuffer[kPpuOverlaySource_Obj] && pitch==g_ppu->overlayRenderPitch[kPpuOverlaySource_Obj],
            "shared HUD restores the host's original overlay binding");
      size_t drawn=RtlSaveSnapshotToMemory(work,capacity);
      if(!m){reference=drawn;memcpy(expected,work,drawn);}
      else same(expected,reference,work,drawn,"HUD layout leaves complete machine, OAM and native overflow flags unchanged");
      if(m==1)memcpy(image,g_ppu->renderBuffer,256u*224u*4u);
      if(m==2)check(memcmp(image,g_ppu->renderBuffer,256u*224u*4u)!=0,"compact and side-by-side HUDs produce their distinct layouts");
    }
    }
    g_ppu->renderFlags=render_flags;
    g_config.widescreen=wide;X3CoopHudSetLayout(X3_COOP_HUD_COMPACT);
  }
  free(equipped);free(image);check(RtlLoadSnapshotFromMemory(original,size),"HUD probes restore their starting machine");
}

/* Observe actual damage, earned refill and survivor fields through both
 * shipped shared layouts and the split compositor. Native per-owner HUD
 * rasters already have a separate original-PPU pixel oracle. */
static unsigned hud_lifecycle_case;
static void hud_lifecycle_picture(uint8_t *image,uint8_t *ramcopy,unsigned tag){
  const X3CoopPresentedFrame *shown=X3CoopPresentationVisible();
  uint16_t meter[2][X3_COOP_HUD_WIDTH*X3_COOP_HUD_HEIGHT];
  memcpy(ramcopy,g_ram,0x20000);uint64_t clock=g_cpu.master_cycles;
  for(unsigned mode=0;mode<(tag==404?3:6);++mode){
    bool split=mode==2 || mode==5,horizontal=mode==1 || mode==3;unsigned panel=mode>=3?342:256;
    X3Display_SetPresentationSeat(-1);X3Display_SetLocalSplit(split);X3Display_SetCoopPanelWidth(panel);
    X3CoopHudSetLayout(horizontal?X3_COOP_HUD_HORIZONTAL:X3_COOP_HUD_COMPACT);
    g_rtl_game_info->draw_ppu_frame();unsigned width=panel*(split?2:1);
    if(split || panel>256)check(X3Display_DrawCoopFrame(image,width*4,NULL,width,224,1),"live HUD adaptive composition presents the actual selected width and fields");
    else{check(g_ppu->renderPitch==256*4,"live shared HUD presents an actual native-width field");memcpy(image,g_ppu->renderBuffer,256u*224u*4u);}
    if(tag==404 && getenv("X3_COOP_HUD_SURVIVOR_VIEW")){
      if(mode==0)memcpy(image+512u*224u*4u,image,256u*224u*4u);
      if(mode==2){unsigned anchor=shown->players.anchor,differences=0;
        check(shown->views[anchor][0]==shown->camera[0] && shown->views[anchor][1]==shown->camera[1],"survivor world comparison uses identical actual Shared and owning cameras");
        for(unsigned y=0;y<224;++y)for(unsigned x=40;x<256;++x)differences+=memcmp(image+512u*224u*4u+(y*256+x)*4,image+(y*512+anchor*256+x)*4,3)!=0;
        fprintf(stderr,"native SURVIVOR WORLD anchor%u camera%u,%u Shared/Split nonHUD differences%u\n",anchor,shown->camera[0],shown->camera[1],differences);
        check(!differences,"unchanged native survivor world pixels match exactly across Shared and the owning split view");
      }
    }
    unsigned opaque=0,errors=0,first_x=0,first_y=0;
    for(unsigned seat=0;seat<2;++seat){
      bool inverted=!split && !horizontal && shown->players.players[seat].character==X3_COOP_ZERO;
      X3CoopHudPlayer(g_ppu,shown,seat,g_snes->cart->rom,g_snes->cart->romImageSize,inverted,meter[seat]);
      unsigned ox=8+(split?seat*panel:horizontal?seat*32:0),oy=inverted?100:0;
      for(unsigned y=0;y<96;++y)for(unsigned x=0;x<32;++x){unsigned pixel=meter[seat][y*32+x];if(!pixel)continue;++opaque;
        const uint8_t *actual=image+((oy+y)*width+ox+x)*4;bool equal=true;
        for(unsigned c=0;c<3;++c)equal=equal && actual[2-c]==g_ppu->brightnessMult[(pixel>>(c*5))&31];
        if(!equal){if(!errors){first_x=ox+x;first_y=oy+y;}++errors;}
      }
    }
    const char *out=getenv("X3_COOP_HUD_LIFECYCLE_OUTPUT");if(out && *out && hud_lifecycle_case<3 && (tag==104 || tag==204 || tag==312 || tag==344 || tag==404)){char path[1024];snprintf(path,sizeof(path),"%s-case%u-field%u-mode%u.ppm",out,hud_lifecycle_case,tag,mode);WritePpm(path,image,width*4,width,224,0,false);}
    if(errors)fprintf(stderr,"live HUD pixels tag%u mode%u errors%u first%u,%u shownHP%u/%u nativeHP%u/%u lives%u/%u\n",tag,mode,errors,first_x,first_y,shown->players.players[0].body[0x27]&127,shown->players.players[1].body[0x27]&127,contact_hp(0),contact_hp(1),shown->players.players[0].life,shown->players.players[1].life);
    check(opaque>100 && !errors,"each submitted personal health/charge/weapon raster occupies its exact shared or split HUD position");
  }
  check(clock==g_cpu.master_cycles && !memcmp(ramcopy,g_ram,0x20000),"all actual live HUD layouts leave native time and shared guest RAM unchanged");
  X3Display_SetLocalSplit(false);X3CoopHudSetLayout(X3_COOP_HUD_COMPACT);
}
static uint8_t *hud_energy_image,*hud_energy_ram;
static unsigned hud_energy_changes,hud_energy_previous,hud_energy_native;
static void hud_weapon_refill_field(unsigned f){
  const X3CoopPresentedFrame *shown=X3CoopPresentationVisible();unsigned energy=shown->weapon_energy[1]&31,native=g_ram[0x1fbc]&31;
  if(!f){hud_energy_changes=0;hud_energy_previous=energy;}
  else{check(energy==hud_energy_native || energy==native,"submitted native weapon energy stays within the actual adjacent producer fields");hud_energy_changes+=energy!=hud_energy_previous;}
  hud_energy_previous=energy;hud_energy_native=native;hud_lifecycle_picture(hud_energy_image,hud_energy_ram,500+f);
  if(f==39){check(energy==16 && hud_energy_changes>=2,"the owning X meter publishes actual native weapon-refill steps through completion");fprintf(stderr,"native LIVE HUD ENERGY changes%u native8->16 layouts6 exactPixels\n",hud_energy_changes);}
}
static void native_hud_lifecycle_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *future,size_t capacity){
  uint8_t *image=malloc(896u*224u*4u),*ramcopy=malloc(0x20000);check(image && ramcopy,"live HUD comparison buffers allocate");
  bool wide=g_config.widescreen;g_config.widescreen=false;
  for(unsigned roster=0;roster<2;++roster){
    check(RtlLoadSnapshot(roster?reversed:normal),"live HUD loads the actual native roster");X3CoopViewsConfigure(true);
    size_t initial=RtlSaveSnapshotToMemory(base,capacity);X3CoopState staging=*X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=staging.players[seat].body;unsigned px=384+128*seat,py=1200-(staging.players[seat].character==X3_COOP_ZERO?25:17);b[5]=b[0x22]=(uint8_t)px;b[6]=b[0x23]=(uint8_t)(px>>8);b[8]=b[0x24]=(uint8_t)py;b[9]=b[0x25]=(uint8_t)(py>>8);b[2]=b[3]=b[4]=b[7]=b[0x30]=b[0x88]=0;memset(b+0x1a,0,6);b[0x27]=staging.players[seat].character==X3_COOP_ZERO?28:16;}
    check(X3CoopEncode(&staging,base+contact_context_offset(base,initial),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,initial),"live damage HUD supplies separated healthy actors on the actual native floor");door_install_player(&staging);
    memset(g_ram+0xd18,0,0x3c0);memset(g_ram+0x1318,0,0x500);
    for(unsigned col=0;col<255;++col){unsigned first=view_read(0xf800+2*col),end=view_read(0xf802+2*col);if(first<0xfa02 || end>0xfffc || end<first || (end-first)%5)continue;for(unsigned at=first;at<end;at+=5)g_ram[at]=1;}
    for(unsigned f=0;f<3;++f)frame(0);size_t neutral=RtlSaveSnapshotToMemory(base,capacity);
    /* Actual native nonfatal damage and fatal handoff, with the other player
     * untouched; low HP supplies only the fatal-contact prerequisite. */
    for(unsigned victim=0;victim<2;++victim){
      hud_lifecycle_case=roster*3+victim;
      check(RtlLoadSnapshotFromMemory(base,neutral),"live HUD restores the native unhurt pair");
      unsigned other=victim^1,old_hp=contact_hp(victim),other_hp=contact_hp(other);CpuState caller=g_cpu;
      contact_geometry(0xd18,support_position(victim,5),support_position(victim,8));contact_native_call(0xd18,0x04cb31);
      uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
      check(contact_hp(victim)<old_hp && contact_hp(victim)>0 && contact_hp(other)==other_hp,"actual native damage lowers only the touched actor's health");
      unsigned damaged=contact_hp(victim);for(unsigned f=0;f<24;++f){frame(0);hud_lifecycle_picture(image,ramcopy,100+f);if(f>=2)check((X3CoopPresentationVisible()->players.players[victim].body[0x27]&127)==damaged,"native damaged health reaches its own submitted meter without a stale health bar");}
      check(RtlLoadSnapshotFromMemory(base,neutral),"fatal HUD contact restores a fresh vulnerable native pose");size_t cut=RtlSaveSnapshotToMemory(work,capacity);X3CoopState setup=*X3CoopRuntimeState();setup.players[victim].body[0x27]=1;setup.players[victim].body[0x30]=0;setup.players[victim].body[0x88]=0;
      check(X3CoopEncode(&setup,work+contact_context_offset(work,cut),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,cut),"live HUD installs only a vulnerable low-health prerequisite");door_install_player(&setup);
      caller=g_cpu;contact_geometry(0xd18,support_position(victim,5),support_position(victim,8));contact_native_call(0xd18,0x04cb31);elapsed=g_cpu.master_cycles;cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
      check(X3CoopRuntimeState()->players[victim].life==X3_COOP_FALLEN && X3CoopRuntimeState()->anchor==other && contact_hp(other)==other_hp,"actual fatal damage transfers the world while preserving the surviving actor's own health");
      for(unsigned f=0;f<32;++f){frame(0);hud_lifecycle_picture(image,ramcopy,200+f);if(f>=2){const X3CoopState *shown=&X3CoopPresentationVisible()->players;check(shown->players[victim].life==X3_COOP_FALLEN && (shown->players[other].body[0x27]&127)==other_hp,"shared and split meters publish the actual corpse and unchanged survivor");}}
      fprintf(stderr,"native LIVE HUD roster%u victim%u damage%u->%u survivor%u HP%u hurt24 fatal32 layouts6 native/adaptive exactPixels\n",roster,victim,old_hp,damaged,other,other_hp);
    }
    check(RtlLoadSnapshot(roster?reversed:normal),"live refill HUD restores the actual unvisited authored item world");neutral=RtlSaveSnapshotToMemory(base,capacity);unsigned seat=health_pickup_approach(base,neutral,work,20),other=seat^1,changes=0,previous=20,freeze=0;X3CoopState placement=*X3CoopRuntimeState();
    /* The reversed movement fixture leaves X mid-air-dash over a later gap.
     * Supply the waiting partner on the known starting floor; native terrain
     * still establishes grounding and the actual item decides every refill. */
    campaign_position_only(&placement.players[other],384,1183);placement.players[other].body[2]=placement.players[other].body[3]=0;
    size_t waiting=RtlSaveSnapshotToMemory(work,capacity);check(X3CoopEncode(&placement,work+contact_context_offset(work,waiting),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,waiting),"refill HUD supplies a waiting partner instead of an unfinished route air dash");door_install_player(&placement);
    /* This debug teleport bypasses the prior real route. Invalidate only its
     * cached camera frontier so the original first-view admission runs. */
    X3CoopViewState admission=*X3CoopViewsState();admission.camera_valid=0;X3CoopViewsSetState(&admission);vehicle_install_view(&admission,placement.anchor);hud_lifecycle_case=roster*3+2;
    for(unsigned f=0;f<100;++f){frame(0);hud_lifecycle_picture(image,ramcopy,300+f);unsigned hp=X3CoopPresentationVisible()->players.players[seat].body[0x27]&127;
      if(hp<20 || hp>28 || contact_hp(other)!=16)fprintf(stderr,"live refill prerequisite observation roster%u f%u collector%u shownHP%u nativeHP%u waitingHP%u waitingXY%u,%u life%u/%u\n",roster,f,seat,hp,contact_hp(seat),contact_hp(other),support_position(other,5),support_position(other,8),X3CoopRuntimeState()->players[seat].life,X3CoopRuntimeState()->players[other].life);
      check(hp>=20 && hp<=28 && contact_hp(other)==16,"native refill HUD remains in the collector's real health range and preserves the partner");changes+=hp!=previous;previous=hp;freeze+=g_ram[0x1f28]!=0;
      if(f>60 && hp==28 && !g_ram[0x1f28])break;
    }
    fprintf(stderr,"live refill observation roster%u changes%u frozen%u shownHP%u nativeHP%u camera%u,%u phase%u/%u\n",roster,changes,freeze,previous,contact_hp(seat),X3CoopViewsState()->camera[seat][0],X3CoopViewsState()->camera[seat][1],g_ram[0xd1],g_ram[0xd2]);
    check(changes>=4 && freeze && previous==28,"the authored refill publishes multiple actual healing steps and its native completion");
    fprintf(stderr,"native LIVE HUD refill roster%u collector%u changes%u frozen%u HP20->28 layouts6 native/adaptive exactPixels\n",roster,seat,changes,freeze);
    /* Reuse the qualified native energy constructor/refill and complete replay;
     * observe only the actual displayed meters during those same fields. */
    check(RtlLoadSnapshot(roster?reversed:normal),"weapon HUD refill restores the actual native roster");neutral=RtlSaveSnapshotToMemory(base,capacity);X3CoopState resting=*X3CoopRuntimeState();
    for(unsigned who=0;who<2;++who){campaign_position_only(&resting.players[who],384+128*who,1200-(resting.players[who].character==X3_COOP_ZERO?25:17));resting.players[who].body[2]=resting.players[who].body[3]=0;}
    check(X3CoopEncode(&resting,base+contact_context_offset(base,neutral),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(base,neutral),"weapon HUD supplies settled approach prerequisites instead of an unfinished route air dash");door_install_player(&resting);vehicle_install_view(X3CoopViewsState(),resting.anchor);for(unsigned f=0;f<3;++f)frame(0);neutral=RtlSaveSnapshotToMemory(base,capacity);
    hud_energy_image=image;hud_energy_ram=ramcopy;pickup_frame_observer=hud_weapon_refill_field;
    native_weapon_pickup_checks(base,neutral,work,future,capacity);pickup_frame_observer=NULL;hud_energy_image=hud_energy_ram=NULL;

  }
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);g_config.widescreen=wide;free(image);free(ramcopy);
}

static void native_hud_survivor_view(const char *fixture){
  check(RtlLoadSnapshot(fixture),"survivor view loads an existing actually fallen native world without placement edits");X3CoopRuntimeFrame();
  bool wide=g_config.widescreen;g_config.widescreen=false;X3Display_SetPresentationSeat(-1);X3Display_SetLocalSplit(false);for(unsigned f=0;f<3;++f)frame(0);
  const X3CoopState *s=X3CoopRuntimeState();check(s->players[s->anchor].life==X3_COOP_ALIVE && s->players[s->anchor^1].life==X3_COOP_FALLEN,"original survivor picture retains exactly one actual living actor");
  uint8_t *image=malloc(768u*224u*4u),*ramcopy=malloc(0x20000);check(image && ramcopy,"survivor view comparison buffers allocate");hud_lifecycle_case=0;hud_lifecycle_picture(image,ramcopy,404);free(image);free(ramcopy);g_config.widescreen=wide;
}
