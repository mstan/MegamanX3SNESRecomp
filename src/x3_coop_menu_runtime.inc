/* Native pause continuation. Included after the actor/world helpers so these
 * operations share their projection and register ownership contracts. */
enum { MENU_BYTES=X3_COOP_MENU_STATE_BYTES };
typedef struct PairedMenu {
  uint32_t frame,handled;
  uint16_t held[2],edge[2];
  uint8_t phase,owner,modal,op,seat,commit,valid,handled_valid;
  uint8_t cursor[2],selection[2],direction[2],repeat[2],heal[2][4],todo[2];
  Registers entry;
  uint8_t ui[64];
} PairedMenu;
static PairedMenu menu;
static bool menu_pending(void){return menu.phase!=0;}
void X3CoopRuntimeInputFrame(uint32_t serial){
  if(!enabled || (menu.valid && menu.frame==serial))return;
  menu.valid=1;menu.frame=serial;
  for(unsigned seat=0;seat<2;++seat){
    unsigned held=X3CoopActions(RtlGetPadState(seat),g_ram+0xffc0);
    menu.edge[seat]=(uint16_t)(held&~menu.held[seat]);menu.held[seat]=(uint16_t)held;
  }
}
bool X3CoopMenuRead(X3CoopMenuInfo *out){
  if(!out || !enabled || !state.initialized || menu.phase<2 || !X3CoopNativePauseActive(g_ram))return false;
  *out=(X3CoopMenuInfo){.owner=menu.owner,.modal=menu.modal};
  for(unsigned seat=0;seat<2;++seat){out->cursor[seat]=menu.cursor[seat];
    out->selection[seat]=menu.selection[seat];out->healing[seat]=menu.heal[seat][1]!=0;}
  return true;
}
static bool menu_choice(unsigned seat,unsigned choice){
  if(choice>=10)return choice<16;
  return state.players[seat].character==X3_COOP_X &&
      (!choice || (g_ram[0x1fbb+choice*2-1]&64));
}
static void menu_restore(CpuState *cpu){
  memcpy(g_ram+0x1ed8,menu.ui,sizeof(menu.ui));restore(cpu,&menu.entry);
  X3CoopProject(&state,g_ram,menu.owner);
  menu.op=menu.seat=0;
}
static void menu_continue(CpuState *cpu){
  for(unsigned seat=0;seat<2;++seat)if(menu.todo[seat]){
    menu.op=menu.todo[seat];menu.seat=(uint8_t)seat;menu.todo[seat]=0;
    restore(cpu,&menu.entry);X3CoopProject(&state,g_ram,seat);
    memcpy(g_ram+0x1ed8,menu.ui,sizeof(menu.ui));
    memcpy(g_ram+0x1f02,menu.heal[seat],4);
    cpu->X=(uint16_t)(menu.op==1?menu.cursor[seat]-10:menu.heal[seat][2]);
    interp_bridge_pre_opcode_redirect(0x00c000|(menu.op==1?0x08b2:0x0901));return;
  }
  menu_restore(cpu);interp_bridge_pre_opcode_redirect(0x00c70f);
}
static bool menu_healing(void){return menu.heal[0][1] || menu.heal[1][1];}
static void menu_controls(CpuState *cpu){
  bool resume=false;
  for(unsigned seat=0;seat<2;++seat){
    if(state.players[seat].life!=X3_COOP_ALIVE)continue;
    unsigned edge=menu.edge[seat],held=menu.held[seat];
    if(menu.modal){
      if(menu.modal!=seat+1)continue;
      if(edge&0x8000){menu.modal=0;continue;}
      if((edge&0x4000) && !menu_healing()){
        menu.entry=regs(cpu);memcpy(menu.ui,g_ram+0x1ed8,sizeof(menu.ui));
        menu.op=3;menu.seat=(uint8_t)seat;X3CoopProject(&state,g_ram,seat);
        interp_bridge_pre_opcode_redirect(0x00cef4);return;
      }
      continue;
    }
    unsigned direction=held&0x0f00;
    if(direction && (direction&(direction-1)))direction=0;
    direction>>=8;
    bool move=direction && (direction!=menu.direction[seat] || !menu.repeat[seat]);
    if(direction!=menu.direction[seat])menu.repeat[seat]=18;
    else if(menu.repeat[seat])--menu.repeat[seat];
    if(move){int step=direction==8 || direction==2?-1:1;
      for(unsigned tries=0;tries<16;++tries){menu.cursor[seat]=(uint8_t)((menu.cursor[seat]+step+16)%16);
        if(menu_choice(seat,menu.cursor[seat]))break;}
      if(direction==menu.direction[seat])menu.repeat[seat]=5;
    }
    menu.direction[seat]=(uint8_t)direction;
    if(edge&0x1000)resume=true;
    if(menu.heal[seat][1])menu.todo[seat]=2;
    if(!(edge&0x4000))continue;
    unsigned choice=menu.cursor[seat];
    if(choice<10){if(menu_choice(seat,choice))menu.selection[seat]=(uint8_t)(choice*2);}
    else if(choice<14 && !menu.heal[seat][1]){
      unsigned tank=choice-10,other=seat^1,amount=g_ram[0x1fb7+tank]&31;
      bool locked=(menu.heal[other][1] && menu.heal[other][2]==tank) ||
          (menu.todo[other]==1 && menu.cursor[other]==choice);
      const X3CoopPlayer *p=&state.players[seat];
      if(!locked && (g_ram[0x1fb7+tank]&128) && amount && amount<=14 &&
          (p->body[0x27]&127)<p->maximum_health)menu.todo[seat]=1;
    }else if(choice==14 && !menu_healing() && !menu.todo[0] && !menu.todo[1]){
      menu.modal=(uint8_t)(seat+1); /* First seat wins simultaneous modal requests. */
    }else if(choice==15)resume=true;
  }
  if(resume && !menu.modal && !menu_healing() && !menu.todo[0] && !menu.todo[1]){
    menu.phase=3;interp_bridge_pre_opcode_redirect(0x00c70b);return;
  }
  menu.entry=regs(cpu);memcpy(menu.ui,g_ram+0x1ed8,sizeof(menu.ui));menu_continue(cpu);
}
static void menu_boundary(CpuState *cpu,uint32_t pc){
  if(!X3CoopRuntimeReady())return;
  switch(pc&0x7fffff){
    case 0x00a2fa:{
      /* Ordinary world ticks keep the original admission path and timing.
       * Only arbitrate a real request or an old single-body pause edge. */
      if(!((menu.edge[0]|menu.edge[1])&0x1000) && !(g_ram[0x1f5d]&0x20) &&
          !(g_ram[X3_COOP_BODY+0x3b]&0x10))return;
      if(menu.phase || scene.phase || refill.active || stock_exit_pending ||
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
      menu.phase=1;menu.owner=(uint8_t)owner;
      /* Co-op participation owns Zero's availability; a native character
       * exchange must not replace the other stored actor during pause. */
      g_ram[0x1f5d]&=(uint8_t)~0x20;
      X3CoopProject(&state,g_ram,owner);g_ram[X3_COOP_BODY+0x3b]|=0x10;
      return;
    }
    case 0x00a36f:
      if(menu.phase==1 && !X3CoopNativePauseActive(g_ram)){menu.phase=0;X3CoopProject(&state,g_ram,state.anchor);}
      return;
    case 0x00c6d6:
      if(!menu.phase)return;
      if(menu.phase==1){
        menu.phase=2;menu.handled_valid=0;
        for(unsigned seat=0;seat<2;++seat){
          unsigned selected=state.players[seat].character==X3_COOP_X?state.players[seat].body[0x33]:0;
          menu.selection[seat]=(uint8_t)(selected<=18 && !(selected&1)?selected:0);
          menu.cursor[seat]=state.players[seat].character==X3_COOP_X?menu.selection[seat]/2:10;
          memset(menu.heal[seat],0,4);menu.direction[seat]=menu.repeat[seat]=menu.todo[seat]=0;
        }
        menu.modal=0;
      }
      if(menu.phase!=2 || (menu.handled_valid && menu.handled==menu.frame)){
        interp_bridge_pre_opcode_redirect(0x00c70f);return;}
      menu.handled_valid=1;menu.handled=menu.frame;menu_controls(cpu);return;
    case 0x00c8d0:case 0x00c93e:
      if(menu.op!=1 && menu.op!=2)return;
      X3CoopCapture(&state,g_ram);memcpy(menu.heal[menu.seat],g_ram+0x1f02,4);
      if(!menu.heal[menu.seat][1])memset(menu.heal[menu.seat],0,4);
      menu_continue(cpu);return;
    case 0x00cf13:case 0x00cf1c:
      if(menu.op!=3)return;
      {bool allowed=(cpu->A&255)!=0;menu_restore(cpu);menu.modal=0;
        if(allowed){menu.phase=3;interp_bridge_pre_opcode_redirect(0x00c6f8);}
        else interp_bridge_pre_opcode_redirect(0x00c70f);}
      return;
    case 0x00ceac:
      /* Retail compacts all remaining reserves when a heal ends. Leave a
       * live tank's index alone; the last native heal will compact once. */
      if(menu.op==2 && (menu.heal[menu.seat^1][1] || menu.todo[menu.seat^1]))
        interp_bridge_pre_opcode_redirect(0x00cef3);
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
      menu.phase=menu.modal=menu.commit=0;return;
  }
}
static void menu_encode(uint8_t out[MENU_BYTES]){
  memset(out,0,MENU_BYTES);memcpy(out,"X3M1",4);uint8_t *p=out+4;
  for(unsigned i=0;i<4;++i)*p++=(uint8_t)(menu.frame>>(8*i));
  for(unsigned i=0;i<4;++i)*p++=(uint8_t)(menu.handled>>(8*i));
  for(unsigned seat=0;seat<2;++seat){putword(p,menu.held[seat]);p+=2;putword(p,menu.edge[seat]);p+=2;}
  *p++=menu.phase;*p++=menu.owner;*p++=menu.modal;*p++=menu.op;
  *p++=menu.seat;*p++=menu.commit;*p++=menu.valid;*p++=menu.handled_valid;
  for(unsigned seat=0;seat<2;++seat){*p++=menu.cursor[seat];*p++=menu.selection[seat];
    *p++=menu.direction[seat];*p++=menu.repeat[seat];memcpy(p,menu.heal[seat],4);p+=4;*p++=menu.todo[seat];}
  putword(p,menu.entry.a);p+=2;putword(p,menu.entry.x);p+=2;putword(p,menu.entry.y);p+=2;
  putword(p,menu.entry.s);p+=2;putword(p,menu.entry.d);p+=2;*p++=menu.entry.db;*p++=menu.entry.p;
  memcpy(p,menu.ui,sizeof(menu.ui));
}
static bool menu_decode(PairedMenu *out,const uint8_t bytes[MENU_BYTES],const X3CoopState *players){
  if(memcmp(bytes,"X3M1",4))return false;PairedMenu v={0};const uint8_t *p=bytes+4;
  for(unsigned i=0;i<4;++i)v.frame|=(uint32_t)*p++<<(8*i);
  for(unsigned i=0;i<4;++i)v.handled|=(uint32_t)*p++<<(8*i);
  for(unsigned seat=0;seat<2;++seat){v.held[seat]=(uint16_t)word(p);p+=2;v.edge[seat]=(uint16_t)word(p);p+=2;}
  v.phase=*p++;v.owner=*p++;v.modal=*p++;v.op=*p++;v.seat=*p++;v.commit=*p++;v.valid=*p++;v.handled_valid=*p++;
  for(unsigned seat=0;seat<2;++seat){v.cursor[seat]=*p++;v.selection[seat]=*p++;
    v.direction[seat]=*p++;v.repeat[seat]=*p++;memcpy(v.heal[seat],p,4);p+=4;v.todo[seat]=*p++;}
  v.entry.a=(uint16_t)word(p);p+=2;v.entry.x=(uint16_t)word(p);p+=2;v.entry.y=(uint16_t)word(p);p+=2;
  v.entry.s=(uint16_t)word(p);p+=2;v.entry.d=(uint16_t)word(p);p+=2;v.entry.db=*p++;v.entry.p=*p++;
  memcpy(v.ui,p,sizeof(v.ui));p+=sizeof(v.ui);
  for(;p<bytes+MENU_BYTES;++p)if(*p)return false;
  if(v.phase>3 || v.owner>1 || v.modal>2 || v.op>3 || v.seat>1 || v.commit>1 || v.valid>1 || v.handled_valid>1)return false;
  if((v.op && v.phase!=2) || (v.commit && v.phase!=3) || (v.modal && v.phase!=2) ||
      (!v.op && v.seat) || (v.handled_valid && !v.valid))return false;
  for(unsigned seat=0;seat<2;++seat){unsigned direction=v.direction[seat];const uint8_t *h=v.heal[seat];
    if((v.held[seat]&~0xdfb0u) || (v.edge[seat]&~v.held[seat]))return false;
    if(v.cursor[seat]>15 || v.selection[seat]>18 || (v.selection[seat]&1) || v.repeat[seat]>18 ||
        (direction && direction!=1 && direction!=2 && direction!=4 && direction!=8) || v.todo[seat]>2)return false;
    if(players->initialized && players->players[seat].character==X3_COOP_ZERO && v.selection[seat])return false;
    if(h[1]?(h[0]<1 || h[0]>2 || h[1]>28 || h[2]>3 || h[3]>1 || (!h[3] && h[1]>14)):
        (h[0] || h[2] || h[3]))return false;
    if((h[1] || v.todo[seat]) && (!players->initialized || players->players[seat].life!=X3_COOP_ALIVE || v.phase!=2))return false;
    if(v.todo[seat] && !v.op)return false;
  }
  if(v.heal[0][1] && v.heal[1][1] && v.heal[0][2]==v.heal[1][2])return false;
  if(v.phase){
    if(!players->initialized || players->wipe || players->players[v.owner].life!=X3_COOP_ALIVE)return false;
    unsigned seat=v.commit?(players->players[0].character==X3_COOP_X?0:1):v.op?v.seat:v.owner;
    if(players->current!=seat || players->players[seat].life!=X3_COOP_ALIVE)return false;
    if(v.op && (v.entry.d!=0x1ed8 || (v.entry.p&0x30)!=0x30))return false;
    if(v.op==1 && (v.cursor[v.seat]<10 || v.cursor[v.seat]>13 || v.heal[v.seat][1]))return false;
    if(v.op==2 && !v.heal[v.seat][1])return false;
    if(v.op==3 && v.modal!=v.seat+1)return false;
    for(unsigned seat=0;seat<2;++seat){
      if(v.todo[seat]==1 && (v.cursor[seat]<10 || v.cursor[seat]>13 || v.heal[seat][1]))return false;
      if(v.todo[seat]==2 && !v.heal[seat][1])return false;
    }
  }else if(v.op || v.commit || v.modal)return false;
  *out=v;return true;
}
