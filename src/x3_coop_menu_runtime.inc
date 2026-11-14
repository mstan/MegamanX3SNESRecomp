/* Native pause continuation. Included after the actor/world helpers so these
 * operations share their projection and register ownership contracts. */
enum { MENU_BYTES=X3_COOP_MENU_STATE_BYTES };
typedef struct PairedMenu {
  uint32_t frame,handled;
  uint16_t held[2],edge[2];
  uint8_t phase,owner,modal,op,seat,commit,valid,handled_valid;
  uint8_t cursor[2],selection[2],direction[2],repeat[2],heal[2][4],todo[2];
  uint8_t gameplay_fade;
  Registers entry;
  uint8_t ui[64];
} PairedMenu;
static PairedMenu menu;
#include "x3_coop_native_menu.inc"
static bool menu_pending(void){return menu.phase!=0;}
bool X3CoopMenuGameplayFade(void){return enabled && menu.phase==1 && menu.gameplay_fade;}
void X3CoopRuntimeInputFrame(uint32_t serial){
  if(!enabled || (menu.valid && menu.frame==serial))return;
#if SNESRECOMP_TRACE
  net_world_prerequisites(serial);
  /* Fixed simulation-frame prerequisites for the actual two-peer menu gate.
   * The frame serial is already part of the rollback snapshot. Replaying this
   * frame reapplies the same low HP/acquired reserve, never a healing result. */
  if(serial==3900 && state.initialized && !scene.phase){
    const char *qa=getenv("SNESRECOMP_X3_NET_QA"),*healing=getenv("SNESRECOMP_X3_NET_QA_MENU");
    if(qa && !strcmp(qa,"1") && healing && !strcmp(healing,"1")){
      X3CoopCapture(&state,g_ram);
      for(unsigned seat=0;seat<2;++seat){state.players[seat].body[0x27]=1;state.players[seat].body[0x88]=255;}
      g_ram[X3_COOP_BODY+0x27]=1;g_ram[X3_COOP_BODY+0x88]=255;
      g_ram[0x1fbc]=0x5c;g_ram[0x1fb7]=0x8e;g_ram[0x1fb8]=0x83;g_ram[0x1fb9]=g_ram[0x1fba]=0x80;
      fprintf(stderr,"[x3-net-menu-prerequisite] serial=%u hp=1/1 reserves=14/3\n",serial);
    }
  }
#endif
  menu.valid=1;menu.frame=serial;
  for(unsigned seat=0;seat<2;++seat){
    unsigned held=X3CoopActions(RtlGetPadState(seat),g_ram+0xffc0);
    unsigned edge=held&~menu.held[seat];
    menu.edge[seat]=(uint16_t)(menu.phase==2?menu.edge[seat]|edge:edge);menu.held[seat]=(uint16_t)held;
  }
  if(menu.op>=4)g_snes->ppu->inidisp|=128;
}
bool X3CoopMenuRead(X3CoopMenuInfo *out){
  if(!out || !enabled || !state.initialized || menu.phase<2 || !X3CoopNativePauseActive(g_ram))return false;
  *out=(X3CoopMenuInfo){.owner=menu.owner,.modal=menu.modal};
  for(unsigned seat=0;seat<2;++seat){out->cursor[seat]=menu.cursor[seat];
    out->selection[seat]=menu.selection[seat];out->healing[seat]=menu.heal[seat][1]!=0;}
  return true;
}
static bool menu_healing(void){return menu.heal[0][1] || menu.heal[1][1];}
static void menu_boundary(CpuState *cpu,uint32_t pc){
  if(!X3CoopRuntimeReady())return;
  switch(pc&0x7fffff){
    case 0x00a2fa:{
      /* Ordinary world ticks keep the original admission path and timing.
       * Only arbitrate a real request or an old single-body pause edge. */
      if(!((menu.edge[0]|menu.edge[1])&0x1000) && !(g_ram[0x1f5d]&0x20) &&
          !(g_ram[X3_COOP_BODY+0x3b]&0x10))return;
      if(menu.phase || scene.phase || refill.active || stock_exit_pending || learner_rescue ||
          controller.pass || terrain.pass || contact.native.pass || objects.native.pass ||
          draw.pass || target.active || solid.native.pass || view_bounds.native.pass){
        interp_bridge_pre_opcode_redirect(0x00a36f);return;}
      settle_deaths();unsigned owner=2;
      if(!state.wipe && !g_ram[0x1f4f])for(unsigned seat=0;seat<2;++seat){
        const X3CoopPlayer *p=&state.players[seat];
        if(p->life==X3_COOP_ALIVE && (p->body[0x27]&127) && !(p->body[0x7e]&1) &&
            (menu.edge[seat]&0x1000)){owner=seat;break;}
      }
      if(owner==2){interp_bridge_pre_opcode_redirect(0x00a36f);return;}
      native_menus_reset();menu.phase=1;menu.owner=(uint8_t)owner;menu.gameplay_fade=1;
      /* Co-op participation owns Zero's availability; a native character
       * exchange must not replace the other stored actor during pause. */
      g_ram[0x1f5d]&=(uint8_t)~0x20;
      /* DD59 still draws the gameplay actors before the original fade. Keep
       * its canonical body/CHR together; only route the requesting Start. */
      putword(g_ram+0xa10,menu.held[owner]);putword(g_ram+0xa12,menu.edge[owner]);
      return;
    }
    case 0x00a36f:
      if(menu.phase==1 && !X3CoopNativePauseActive(g_ram)){menu.phase=menu.gameplay_fade=0;X3CoopProject(&state,g_ram,state.anchor);}
      return;
    case 0x00c5ef:
      if(menu.phase==1){
        /* The cartridge has finished fading gameplay to black. Its first
         * menu initializer now owns the requester's body and palette backup. */
        menu.gameplay_fade=0;X3CoopProject(&state,g_ram,menu.owner);
        memcpy(g_ram+X3_COOP_PALETTE,state.players[menu.owner].palette,64);
        g_ram[0xa1]=1;
      }
      return;
    case 0x00cc5b:
      if(menu.phase==1){
        unsigned map=(PPU_bgTilemapAdr(g_snes->ppu,1)&0x7fff)*2;
        memcpy(native_menus.background,(uint8_t *)g_snes->ppu->vram+map,sizeof(native_menus.background));
        native_menus.background_ready=1;
      }
      return;
    case 0x00c6d6:
      if(!menu.phase)return;
      if(menu.op>=4)return;
      if(menu.phase==1){
        menu.phase=2;menu.handled_valid=0;
        for(unsigned seat=0;seat<2;++seat){
          unsigned selected=state.players[seat].character==X3_COOP_X?state.players[seat].body[0x33]:0;
          menu.selection[seat]=(uint8_t)(selected<=18 && !(selected&1)?selected:0);
          menu.cursor[seat]=state.players[seat].character==X3_COOP_X?menu.selection[seat]/2:10;
          memset(menu.heal[seat],0,4);menu.direction[seat]=menu.repeat[seat]=menu.todo[seat]=0;
        }
        menu.modal=0;native_menu_prepare();
      }
      if(menu.phase!=2 || (menu.handled_valid && menu.handled==menu.frame)){
        interp_bridge_pre_opcode_redirect(0x00c70f);return;}
      menu.handled_valid=1;menu.handled=menu.frame;
      menu.entry=regs(cpu);memcpy(menu.ui,g_ram+0x0a10,4);
      cx4_sync(g_snes->cart->cx4,cpu->master_cycles);
      memcpy(native_menus.cx4_before,cx4_ram_ptr(g_snes->cart->cx4,0x6000),sizeof(native_menus.cx4_before));
      unsigned other=menu.owner^1;
      bool visit_other=!(native_menus.primed&(1u<<other)) || state.players[other].life==X3_COOP_ALIVE;
      native_menu_pass(cpu,visit_other?other:menu.owner);
      return;
    case 0x00c5db:
      if(menu.op==5){
        if(++native_menus.init_step<sizeof(native_menu_builders)/sizeof(*native_menu_builders)){
          native_menu_build_step(cpu);return;
        }
        native_menus.init_step=0;native_menus.primed|=(uint8_t)(1u<<menu.seat);
        memcpy(cx4_ram_ptr(g_snes->cart->cx4,0x6000),native_menus.cx4_before,sizeof(native_menus.cx4_before));
        if(state.players[menu.seat].life!=X3_COOP_ALIVE){
          if(!native_menu_uploads()){enabled=false;return;}
          menu.op=6;cpu->D=0;cpu->S=menu.entry.s;interp_bridge_pre_opcode_redirect(0x00dd59);return;
        }
        memset(g_ram+0xe4,0,2);memset(g_ram+0xe7,0,7);
        restore(cpu,&menu.entry);cpu->S=menu.entry.s;menu.op=4;interp_bridge_pre_opcode_redirect(0x00c6d6);return;
      }
      if(menu.op==4){
        if(!native_menu_uploads()){enabled=false;return;}
        /* CD6E queued native object descriptors. Flush that character's
         * actual cartridge OAM once before retaining its private picture. */
        menu.op=6;cpu->D=0;cpu->S=menu.entry.s;interp_bridge_pre_opcode_redirect(0x00dd59);return;
      }
      if(menu.op!=6)return;
      cpu->D=menu.entry.d;
      X3CoopCapture(&state,g_ram);
      {unsigned seat=menu.seat,selected=g_ram[0x1ee2];
        menu.cursor[seat]=(uint8_t)(selected<20?selected/2:selected>=22 && selected<=28?10+(selected-22)/2:14);
        if(state.players[seat].character==X3_COOP_X && selected<=18 && !(selected&1))menu.selection[seat]=(uint8_t)selected;
        memcpy(menu.heal[seat],g_ram+0x1f02,4);
        if(!menu.heal[seat][1])memset(menu.heal[seat],0,4);
        if(g_ram[0x1ed9]==4)native_menus.resume=1;
        native_menu_finish_picture(seat);
        if(seat!=menu.owner){
          memcpy(cx4_ram_ptr(g_snes->cart->cx4,0x6000),native_menus.cx4_before,sizeof(native_menus.cx4_before));
          restore(cpu,&menu.entry);cpu->S=menu.entry.s;native_menu_pass(cpu,menu.owner);
          interp_bridge_pre_opcode_redirect(0x00c6d6);return;
        }}
      memcpy(g_ram+0x0a10,menu.ui,4);menu.op=menu.seat=0;menu.handled=menu.frame;
      if(native_menus.resume && !menu_healing()){
        g_ram[0x1ed9]=4;if(menu.modal)g_ram[0x1eff]=1;menu.modal=0;menu.phase=3;
      }else if(native_menus.resume){
        g_ram[0x1ed9]=2;menu.modal=0;native_menus.resume=0;
        for(unsigned seat=0;seat<2;++seat){native_menus.view[seat].ui[1]=2;native_menus.view[seat].ui[0x27]=0;}
      }
      g_snes->ppu->inidisp=g_ram[0xb4]&15;
      return;
    case 0x00dd59:
      if(menu.phase && !menu.op && menu.handled_valid && menu.handled==menu.frame)
        interp_bridge_pre_opcode_redirect(0x00dd5d);
      return;
    case 0x008162:
      if(menu.op>=4){
        g_snes->ppu->inidisp|=128;if(!native_menu_uploads()){enabled=false;return;}
        interp_bridge_pre_opcode_redirect(0x00c70f);
      }
      return;
    case 0x00c77f:case 0x00c79b:
      if(menu.op==4 && (cpu->X==20 || state.players[menu.seat].character==X3_COOP_ZERO)){
        cpu_mirrors_to_p(cpu);cpu->P&=(uint8_t)~0x40;cpu_p_to_mirrors(cpu);
      }
      return;
    case 0x00c8a6:
      if(menu.op==4 && !stock_zero && state.players[menu.seat].character==X3_COOP_ZERO){
        cpu_mirrors_to_p(cpu);cpu->P|=2;cpu_p_to_mirrors(cpu);
      }
      return;
    case 0x00c8b2:
      if(menu.op==4 && menu.heal[menu.seat^1][1] && menu.heal[menu.seat^1][2]==cpu->X)
        interp_bridge_pre_opcode_redirect(0x00c8d0);
      return;
    case 0x00c6f8:
      if(menu.op==4){
        if(menu.modal)interp_bridge_pre_opcode_redirect(0x00c70b);
        else menu.modal=(uint8_t)(menu.seat+1);
      }
      return;
    case 0x00ceac:
      /* Retail compacts all remaining reserves when a heal ends. Leave a
       * live tank's index alone; the last native heal will compact once. */
      if(menu.op==4 && menu.heal[menu.seat^1][1])interp_bridge_pre_opcode_redirect(0x00cef3);
      return;
    case 0x00cb64:
      if(menu.phase!=3)return;
      /* CFCB/D0A5 have restored the requesting actor's palette and saved
       * body/armor draw flags. Capture that owner before committing X's
       * actual native equipment change, then restore the world anchor. */
      X3CoopCapture(&state,g_ram);
      {unsigned x=state.players[0].character==X3_COOP_X?0:1;
        /* With only Zero living, the same native routine still restores
         * his normal graphics. There is no X equipment to borrow. */
        if(state.players[x].life!=X3_COOP_ALIVE){g_ram[0x1ee2]=0;return;}
        X3CoopProject(&state,g_ram,x);g_ram[0x1ee2]=menu.selection[x];menu.commit=1;}
      return;
    case 0x00cb67:
      if(menu.phase!=3)return;
      X3CoopCapture(&state,g_ram);X3CoopProject(&state,g_ram,state.anchor);anchor_palette();
      for(unsigned seat=0;seat<2;++seat)state.players[seat].previous_actions=menu.held[seat];
      menu.phase=menu.modal=menu.commit=0;native_menus_reset();return;
  }
}
static void menu_encode(uint8_t out[MENU_BYTES]){
  memset(out,0,MENU_BYTES);memcpy(out,"X3M2",4);uint8_t *p=out+4;
  for(unsigned i=0;i<4;++i)*p++=(uint8_t)(menu.frame>>(8*i));
  for(unsigned i=0;i<4;++i)*p++=(uint8_t)(menu.handled>>(8*i));
  for(unsigned seat=0;seat<2;++seat){putword(p,menu.held[seat]);p+=2;putword(p,menu.edge[seat]);p+=2;}
  *p++=menu.phase;*p++=menu.owner;*p++=menu.modal;*p++=menu.op;
  *p++=menu.seat;*p++=menu.commit;*p++=menu.valid;*p++=menu.handled_valid;
  for(unsigned seat=0;seat<2;++seat){*p++=menu.cursor[seat];*p++=menu.selection[seat];
    *p++=menu.direction[seat];*p++=menu.repeat[seat];memcpy(p,menu.heal[seat],4);p+=4;*p++=menu.todo[seat];}
  putword(p,menu.entry.a);p+=2;putword(p,menu.entry.x);p+=2;putword(p,menu.entry.y);p+=2;
  putword(p,menu.entry.s);p+=2;putword(p,menu.entry.d);p+=2;*p++=menu.entry.db;*p++=menu.entry.p;
  memcpy(p,menu.ui,sizeof(menu.ui));p+=sizeof(menu.ui);*p++=menu.gameplay_fade;
}
static bool menu_decode(PairedMenu *out,const uint8_t bytes[MENU_BYTES],const X3CoopState *players){
  bool legacy=!memcmp(bytes,"X3M1",4);
  if(!legacy && memcmp(bytes,"X3M2",4))return false;PairedMenu v={0};const uint8_t *p=bytes+4;
  for(unsigned i=0;i<4;++i)v.frame|=(uint32_t)*p++<<(8*i);
  for(unsigned i=0;i<4;++i)v.handled|=(uint32_t)*p++<<(8*i);
  for(unsigned seat=0;seat<2;++seat){v.held[seat]=(uint16_t)word(p);p+=2;v.edge[seat]=(uint16_t)word(p);p+=2;}
  v.phase=*p++;v.owner=*p++;v.modal=*p++;v.op=*p++;v.seat=*p++;v.commit=*p++;v.valid=*p++;v.handled_valid=*p++;
  if(legacy && v.phase)return false;
  for(unsigned seat=0;seat<2;++seat){v.cursor[seat]=*p++;v.selection[seat]=*p++;
    v.direction[seat]=*p++;v.repeat[seat]=*p++;memcpy(v.heal[seat],p,4);p+=4;v.todo[seat]=*p++;}
  v.entry.a=(uint16_t)word(p);p+=2;v.entry.x=(uint16_t)word(p);p+=2;v.entry.y=(uint16_t)word(p);p+=2;
  v.entry.s=(uint16_t)word(p);p+=2;v.entry.d=(uint16_t)word(p);p+=2;v.entry.db=*p++;v.entry.p=*p++;
  memcpy(v.ui,p,sizeof(v.ui));p+=sizeof(v.ui);
  v.gameplay_fade=*p++;
  for(;p<bytes+MENU_BYTES;++p)if(*p)return false;
  if(v.phase>3 || v.owner>1 || v.modal>2 || (v.op && v.op!=4 && v.op!=5 && v.op!=6) || v.seat>1 || v.commit>1 || v.valid>1 || v.handled_valid>1)return false;
  if(v.gameplay_fade>1 || (v.gameplay_fade && v.phase!=1))return false;
  if(v.op>=4 && (v.entry.s<0x100 || v.entry.s>0x1fa))return false;
  if((v.op && v.phase!=2) || (v.commit && v.phase!=3) || (v.modal && v.phase!=2) ||
      (!v.op && v.seat) || (v.handled_valid && !v.valid))return false;
  for(unsigned seat=0;seat<2;++seat){unsigned direction=v.direction[seat];const uint8_t *h=v.heal[seat];
    if((v.held[seat]&~0xdfb0u) || (v.edge[seat]&~0xdfb0u) || (legacy && (v.edge[seat]&~v.held[seat])))return false;
    if(v.cursor[seat]>15 || v.selection[seat]>18 || (v.selection[seat]&1) || v.repeat[seat] || direction || v.todo[seat])return false;
    if(players->initialized && players->players[seat].character==X3_COOP_ZERO && v.selection[seat])return false;
    if(h[1]?(h[0]<1 || h[0]>2 || h[1]>28 || h[2]>3 || h[3]>1 || (!h[3] && h[1]>14)):
        (h[0] || h[2] || h[3]))return false;
    if((h[1] || v.todo[seat]) && (!players->initialized || players->players[seat].life!=X3_COOP_ALIVE || v.phase!=2))return false;
    if(v.todo[seat] && !v.op)return false;
  }
  if(v.heal[0][1] && v.heal[1][1] && v.heal[0][2]==v.heal[1][2])return false;
  if(v.phase){
    if(!players->initialized || players->wipe || players->players[v.owner].life!=X3_COOP_ALIVE)return false;
    unsigned seat=v.gameplay_fade?players->anchor:v.commit?(players->players[0].character==X3_COOP_X?0:1):v.op?v.seat:v.owner;
    if(players->current!=seat || (players->players[seat].life!=X3_COOP_ALIVE && v.op!=5 && v.op!=6))return false;
    if(v.op && (v.entry.d!=0x1ed8 || (v.entry.p&0x30)!=0x30))return false;
  }else if(v.op || v.commit || v.modal)return false;
  *out=v;return true;
}
