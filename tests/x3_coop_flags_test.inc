/* Prepared native hit geometry and temporary status are prerequisites, not
 * an assertion of an authored enemy/upgrade acquisition route. */
static unsigned actor_native_flags(unsigned seat){
  return X3CoopRuntimeState()->current==seat?g_ram[0x1fff]:X3CoopRuntimeState()->players[seat].native_flags;
}
static void native_flags_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  uint8_t *prepared=malloc(capacity);check(prepared!=NULL,"native temporary-status replay buffer allocates");
  for(unsigned roster=0;roster<2;++roster)for(unsigned victim=0;victim<2;++victim)for(unsigned protected=0;protected<2;++protected){
    if(roster){const char *path=getenv("X3_COOP_FLAGS_REVERSED_STATE");
      check(path && *path && RtlLoadSnapshot(path),"temporary-status check loads its real reversed native roster");
    }else check(RtlLoadSnapshotFromMemory(original,size),"temporary-status check restores the original native roster");
    X3CoopState setup=*X3CoopRuntimeState();
    for(unsigned seat=0;seat<2;++seat){
      setup.players[seat].body[0x26]=0;
      setup.players[seat].body[0x27]=setup.players[seat].maximum_health;
      setup.players[seat].native_flags=(uint8_t)((setup.players[seat].character==X3_COOP_ZERO?2:0)|
          (setup.players[seat].character==X3_COOP_X && protected?0xc0:0));
    }
    size_t n=RtlSaveSnapshotToMemory(work,capacity);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) &&
          RtlLoadSnapshotFromMemory(work,n),"private native damage-status prerequisite restores");
    door_install_player(&setup);g_ram[0x1fd1]|=4;g_ram[0x1fd7]&=191; /* Native X armor prerequisite. */
    unsigned x=setup.players[victim].body[5]|setup.players[victim].body[6]<<8;
    unsigned y=setup.players[victim].body[8]|setup.players[victim].body[9]<<8;
    contact_geometry(0xd18,x,y);
    g_ram[0xd18+0x26]=4;
    size_t base=RtlSaveSnapshotToMemory(prepared,capacity);
    X3CoopRuntimeEnable(false);X3CoopState solo=setup;solo.current=(uint8_t)victim;door_install_player(&solo);
    contact_native_call(0xd18,0x04cb31);
    unsigned hp=g_ram[X3_COOP_BODY+0x27]&127,flags=g_ram[0x1fff];
    fprintf(stderr,"native status roster%u victim%u protected%u: HP%u->%u flags%02x->%02x result%04x invuln%u\n",
        roster,victim,protected,setup.players[victim].maximum_health,hp,solo.players[victim].native_flags,
        flags,g_cpu.A,g_ram[X3_COOP_BODY+0x26]);
    unsigned damage=setup.players[victim].character==X3_COOP_X?(protected?1:2):4;
    check(hp==setup.players[victim].maximum_health-damage,"original native armor and temporary-status rules affect X while Zero retains native damage");
    check(RtlLoadSnapshotFromMemory(prepared,base),"paired status starts from the exact native contact prerequisite");
    contact_native_call(0xd18,0x04cb31);
    check(contact_hp(victim)==hp && actor_native_flags(victim)==flags,
          "paired native damage and consumed temporary status match the original victim update");
    check(contact_hp(victim^1)==setup.players[victim^1].maximum_health &&
          actor_native_flags(victim^1)==setup.players[victim^1].native_flags,
          "a distant actor neither lends nor loses temporary damage status");
    size_t result=RtlSaveSnapshotToMemory(expected,capacity);
    check(RtlLoadSnapshotFromMemory(prepared,base),"temporary native damage status restores for replay");
    contact_native_call(0xd18,0x04cb31);size_t replay=RtlSaveSnapshotToMemory(work,capacity);
    same(expected,result,work,replay,"private native flags, contact outcome and the complete machine replay identically");
  }
  free(prepared);check(RtlLoadSnapshotFromMemory(original,size),"native status checks restore original gameplay");
}
