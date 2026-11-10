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
