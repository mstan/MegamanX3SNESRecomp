/* Contract probes against the cartridge's real collision routines. These use
 * isolated geometry fixtures, not an assertion of stage/AI coverage. */
#include "../src/x3_coop_render.h"
static size_t contact_context_offset(const uint8_t *snapshot,size_t size) {
  for(size_t i=size-4;i>0;--i)if(!memcmp(snapshot+i,"X3C4",4))return i;
  check(0,"actor context is present");return 0;
}
static void contact_geometry(unsigned slot,unsigned x,unsigned y) {
  /* Both records use the same native geometry-table contract. Enemy motion,
   * AI, spawning and retirement are deliberately not manufactured here. */
  memcpy(g_ram+slot,g_ram+X3_COOP_BODY,64);
  g_ram[slot]=1;g_ram[slot+0xe]=1;g_ram[slot+0x26]=2;g_ram[slot+0x27]=8;
  g_ram[slot+5]=(uint8_t)x;g_ram[slot+6]=(uint8_t)(x>>8);
  g_ram[slot+8]=(uint8_t)y;g_ram[slot+9]=(uint8_t)(y>>8);
  g_ram[slot+0x30]=0;g_ram[0x1f1d]=0;
}
static void contact_native_call(unsigned slot,unsigned pc) {
  X3CoopRuntimeFrame();
  uint16_t stack=g_cpu.S;
  g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=(uint16_t)slot;
  g_cpu.P=(uint8_t)((g_cpu.P|0x30)&~0x40);cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc),"native contact returns");
  check(g_cpu.S==stack,"paired contact preserves the real guest stack");
}
static unsigned contact_hp(unsigned seat) {
  const X3CoopState *s=X3CoopRuntimeState();
  return (s->current==seat?g_ram[X3_COOP_BODY+0x27]:s->players[seat].body[0x27])&127;
}
static void native_health_limit_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  for(unsigned seat=0;seat<2;++seat)for(unsigned full=0;full<2;++full){
    check(RtlLoadSnapshotFromMemory(snapshot,size),"health-limit probe restores");
    X3CoopState setup=*X3CoopRuntimeState();
    check(X3CoopProject(&setup,g_ram,seat),"health probe projects the actor's native globals");
    unsigned maximum=seat?28:16;
    check(g_ram[0x1fd2]==maximum,"native maximum belongs to the projected character");
    g_ram[X3_COOP_BODY+0x27]=(uint8_t)(maximum-1+full);
    g_ram[X3_COOP_BODY+0xbf]=1;g_ram[X3_COOP_BODY+0xc0]=0;
    uint16_t stack=g_cpu.S;g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=X3_COOP_BODY;
    g_cpu.P|=0x30;cpu_p_to_mirrors(&g_cpu);cpu_push_jsr_return_frame(&g_cpu);
    check(interp_bridge_run(&g_cpu,0x04aa09) && g_cpu.S==stack,"native health regeneration returns with a balanced stack");
    check((g_ram[X3_COOP_BODY+0x27]&127)==maximum,"native healing reaches each actor's limit and cannot exceed it");
  }
  check(RtlLoadSnapshotFromMemory(snapshot,size),"health preflight probe restores");
  memcpy(modified,snapshot,size);
  size_t offset=contact_context_offset(snapshot,size);
  modified[offset+X3CoopEncodedSize()-1]=0;
  check(!RtlLoadSnapshotFromMemory(modified,size),"invalid personal health maximum is refused before mutation");
  size_t after=RtlSaveSnapshotToMemory(modified,capacity);
  same(snapshot,size,modified,after,"refused health maximum leaves the machine unchanged");
}
static void native_reversed_roster_checks(uint8_t *snapshot,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  const X3CoopState *s=X3CoopRuntimeState();
  check(s->initialized && s->anchor==1 && s->current==1 &&
        s->players[0].character==X3_COOP_ZERO && s->players[1].character==X3_COOP_X,
        "reversed roster keeps the original native X as canonical seat2");
  check(s->players[0].maximum_health==28 && s->players[1].maximum_health==16 &&
        (s->players[0].body[0x27]&127)==28 && (s->players[1].body[0x27]&127)==16 &&
        s->players[0].body[0xb6]==2 && !s->players[1].body[0xb6] && g_ram[0x1fd2]==16,
        "reversed roster preserves native character selectors, health and world maximum");
  unsigned zero=coop_x(0),x=coop_x(1);
  for(unsigned i=0;i<30;++i)frame(SNES_PAD_RIGHT);
  check(coop_x(0)>zero && coop_x(1)==x,"seat1 Zero moves without borrowing X input");
  zero=coop_x(0);
  for(unsigned i=0;i<30;++i)frame(SNES_PAD_RIGHT<<12);
  check(coop_x(0)==zero && coop_x(1)>x,"seat2 X moves without borrowing Zero input");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"reversed roster replay start restores");
  bool shot=false;
  for(unsigned i=0;i<30;++i){
    frame(SNES_PAD_Y | (SNES_PAD_RIGHT<<12));s=X3CoopRuntimeState();
    for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)if(s->players[0].shots[j])shot=true;
  }
  size_t n=RtlSaveSnapshotToMemory(expected,capacity);
  check(shot,"reversed Zero owns his native projectile pool");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"reversed roster repeat start restores");
  for(unsigned i=0;i<30;++i)frame(SNES_PAD_Y | (SNES_PAD_RIGHT<<12));
  size_t actual=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,n,work,actual,"reversed roster movement/combat deterministic replay");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"reversed roster probes restore");
}
static void contact_native_draw(unsigned slot,unsigned expected_x,unsigned expected_y) {
  uint16_t stack=g_cpu.S;
  unsigned descriptor=cpu_read8(&g_cpu,0,0x6620);
  check(descriptor<128,"native draw fixture has descriptor capacity");
  g_cpu.PB=3;g_cpu.DB=0;g_cpu.D=0;g_cpu.X=(uint16_t)(slot|0x8000);
  g_cpu.P=(uint8_t)((g_cpu.P|0x20)&~0x10);cpu_p_to_mirrors(&g_cpu);
  cpu_push_jsr_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x038f70),"tagged native late draw returns");
  check(g_cpu.S==stack,"native late draw preserves guest return frame");
  check(cpu_read16(&g_cpu,0,0x6220+descriptor*16)==expected_x &&
        cpu_read16(&g_cpu,0,0x6222+descriptor*16)==expected_y,
        "native late descriptor reads the partner's real shot coordinates");
  check(X3CoopRuntimeState()->current==X3CoopRuntimeState()->anchor,
        "native late draw restores the world actor");
}
static void native_contact_checks(uint8_t *snapshot,size_t size,uint8_t *modified,size_t capacity) {
  size_t offset=contact_context_offset(snapshot,size);
  check(offset+X3CoopEncodedSize()<=size,"actor chunk fits snapshot");
  X3CoopState setup;
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"native fixture context decodes");
  unsigned primary_x=setup.players[0].body[5]|setup.players[0].body[6]<<8;
  unsigned partner_x=setup.players[1].body[5]|setup.players[1].body[6]<<8;
  unsigned y=setup.players[1].body[8]|setup.players[1].body[9]<<8;
  check(primary_x!=partner_x,"native fixture separates both bodies");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"isolated P2 contact fixture restores");
  contact_geometry(0xd18,partner_x,y);contact_native_call(0xd18,0x04cb31);
  check((g_cpu.A&255)!=0 && contact_hp(0)==16 && contact_hp(1)==26,
        "native enemy body contact reaches P2 without damaging distant X");

  memcpy(modified,snapshot,size);
  setup.players[1].body[5]=setup.players[0].body[5];
  setup.players[1].body[6]=setup.players[0].body[6];
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"overlap fixture encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"overlapping bodies restore");
  contact_geometry(0xd18,primary_x,y);contact_native_call(0xd18,0x04cb31);
  check(contact_hp(0)==14 && contact_hp(1)==26,"solid enemy contact evaluates each eligible body once");

  check(RtlLoadSnapshotFromMemory(modified,size),"hostile-shot overlap fixture restores");
  contact_geometry(0x1318,primary_x,y);contact_native_call(0x1318,0x04cb31);
  check(contact_hp(0)==14 && contact_hp(1)==28,"consumed hostile shot does not damage both players");
  check(X3CoopDecode(&setup,snapshot+offset,X3CoopEncodedSize()),"fatal-contact context decodes");
  setup.players[1].body[0x27]=1;
  memcpy(modified,snapshot,size);
  check(X3CoopEncode(&setup,modified+offset,X3CoopEncodedSize()),"fatal P2 precondition encodes");
  check(RtlLoadSnapshotFromMemory(modified,size),"fatal P2 contact fixture restores");
  contact_geometry(0xd18,partner_x,y);contact_native_call(0xd18,0x04cb31);
  check(X3CoopRuntimeState()->players[1].life==X3_COOP_FALLEN &&
        X3CoopRuntimeState()->current==0 && contact_hp(0)==16,
        "fatal partner contact settles before the enemy continuation resumes");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"fatal X contact fixture restores");
  g_ram[X3_COOP_BODY+0x27]=1;
  contact_geometry(0xd18,primary_x,y);contact_native_call(0xd18,0x04cb31);
  check(X3CoopRuntimeState()->players[0].life==X3_COOP_FALLEN &&
        X3CoopRuntimeState()->current==1 && (g_ram[X3_COOP_BODY+0x27]&127)==28 && g_ram[0x1fd2]==28,
        "native fatal-contact continuation sees the living survivor's HP");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"native projectile-update fixture restores");
  for(int i=0;i<3;++i)frame(SNES_PAD_Y<<12);
  const uint8_t *shot=X3CoopRuntimeState()->players[1].shots;
  unsigned index=0;
  while(index<X3_COOP_SHOT_BYTES && !shot[index])index+=64;
  check(index<X3_COOP_SHOT_BYTES,"native partner creates a buster projectile");
  unsigned before=shot[index+5]|shot[index+6]<<8;
  for(int i=0;i<3;++i)frame(0);
  shot=X3CoopRuntimeState()->players[1].shots;
  check(shot[index] && (unsigned)(shot[index+5]|shot[index+6]<<8)!=before,
        "partner projectile advances through its native updater");
  bool tagged=false;
  for(unsigned p=0x700;p<0x880;p+=2) {
    unsigned request=g_ram[p]|g_ram[p+1]<<8;
    if(request==(0x8000|(X3_COOP_SHOTS+index)))tagged=true;
  }
  check(tagged,"native partner update retains ownership in its late draw queue");
  contact_native_draw(X3_COOP_SHOTS+index,shot[index+5]|shot[index+6]<<8,
                      shot[index+8]|shot[index+9]<<8);
  contact_geometry(0xd18,shot[index+5]|shot[index+6]<<8,shot[index+8]|shot[index+9]<<8);
  g_ram[0xd18+0x28]=0;
  contact_native_call(0xd18,0x04cb74);
  check((g_ram[0xd18+0x27]&127)<8,"partner projectile damages shared native enemy");
  check(RtlSaveSnapshotToMemory(modified,capacity)!=0,"updated native pools remain snapshot-capable");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"native charge palette fixture restores");
  uint8_t primary_palette[X3_COOP_PALETTE_BYTES],partner_palette[X3_COOP_PALETTE_BYTES];
  memcpy(primary_palette,X3CoopRuntimeState()->players[0].palette,sizeof(primary_palette));
  memcpy(partner_palette,X3CoopRuntimeState()->players[1].palette,sizeof(partner_palette));
  bool isolated=true,animated=false;
  for(unsigned i=0;i<180;++i) {
    frame(SNES_PAD_Y<<12);
    const X3CoopState *s=X3CoopRuntimeState();
    isolated=isolated && !memcmp(primary_palette,s->players[0].palette,sizeof(primary_palette));
    animated=animated || memcmp(partner_palette,s->players[1].palette,sizeof(partner_palette));
  }
  check(isolated && animated,"Zero's native charge ramps animate without recoloring X");
  uint16_t emulated_colors[256];memcpy(emulated_colors,g_ppu->cgram,sizeof(emulated_colors));
  X3CoopRender();X3CoopRenderRestorePalette();
  check(!memcmp(emulated_colors,g_ppu->cgram,sizeof(emulated_colors)),
        "player presentation restores exact emulated CGRAM");
  check(RtlLoadSnapshotFromMemory(snapshot,size),"contact probes restore their starting machine");
}
