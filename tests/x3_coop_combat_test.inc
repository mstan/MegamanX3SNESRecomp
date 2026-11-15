/* Isolated native combat: the actual controller/owned-pool loops run normally.
 * Clear shared hostile slots and consume local spawn visits as prerequisites;
 * no attack, charge, capacity, hit or progression result is supplied. */
typedef struct CombatCues {uint8_t count,ids[32],pan[32];} CombatCues;
static CombatCues combat_cues,combat_reference_cues[2][420];
static uint16_t combat_reference_camera[2][420][6];
static bool combat_audio_enabled,combat_visual_enabled;
typedef struct CombatPicture {uint8_t chr[8192],palette[64];} CombatPicture;
enum {COMBAT_VISUAL_FRAMES=1200};
static CombatPicture combat_pictures[2][COMBAT_VISUAL_FRAMES];
static unsigned combat_hit_cues;
static void combat_audio_install(void){combat_audio_enabled=getenv("X3_COOP_COMBAT_AUDIO")!=NULL;combat_visual_enabled=getenv("X3_COOP_COMBAT_VISUAL")!=NULL;}
static const uint8_t *combat_rom(unsigned address,unsigned count){
  size_t at=((address>>16)&127)*32768+(address&32767),size=g_snes->cart->romImageSize;
  check((address&65535)>=32768 && at<=size && count<=size-at,"visual reference reads only the validated native cartridge");return g_snes->cart->rom+at;
}
/* Inherit only the current native character's body cells. Replacing the
 * entire8KiB bank also erases common projectile art the real character swap
 * retains (for example the buster tile at6EF0). Later poses use native DMA. */
static void combat_seed_body(unsigned seat,const uint8_t *body){
  const uint8_t *p=combat_rom(0x8d8000+body[0x16]*3,3);unsigned at=p[0]|p[1]<<8|p[2]<<16;
  p=combat_rom(at+(body[0x17]&127)*3,3);at=p[0]|p[1]<<8|p[2]<<16;unsigned count=*combat_rom(at,1);
  check(count && count<=64,"original visual reference inherits a real native body layout");p=combat_rom(at+1,count*4);
  const uint8_t *atlas=X3CoopPresentationVisible()->body_chr[seat];
  for(unsigned piece=0;piece<count;++piece){const uint8_t *cell=p+piece*4;unsigned side=cell[0]&32?2:1,tile=cell[3];
    for(unsigned y=0;y<side;++y)for(unsigned x=0;x<side;++x){unsigned index=(((tile>>4)+y)&15)*16+((tile+x)&15);memcpy(g_ppu->vram+0x6000+index*16,atlas+index*32,32);}
  }
}
/* Compare the actual source pixels consumed by owned FX with each original
 * actor's native VRAM/CGRAM, while both actors upload and animate together. */
typedef struct CombatEffectPicture {uint16_t vram[32768],colors[128];} CombatEffectPicture;
static CombatEffectPicture *combat_effect_pictures[2];
static unsigned combat_effect_tiles[2],combat_effect_ink[2];
static void combat_effect_capture(unsigned seat,unsigned f){
  if(!getenv("X3_COOP_COMBAT_EFFECT_VISUAL"))return;
  if(!combat_effect_pictures[seat]){combat_effect_pictures[seat]=malloc(COMBAT_VISUAL_FRAMES*sizeof(CombatEffectPicture));check(combat_effect_pictures[seat]!=NULL,"native effect source buffers allocate within their bounded native trace");}
  check(seat<2 && f<COMBAT_VISUAL_FRAMES,"native visual reference stays within its field bound");
  CombatEffectPicture *out=&combat_effect_pictures[seat][f];memcpy(out->vram,g_ppu->vram,sizeof(out->vram));memcpy(out->colors,g_ppu->cgram+128,sizeof(out->colors));
  const char *output=getenv("X3_COOP_EFFECT_OUTPUT");if(output && *output && f==1){char path[1024];snprintf(path,sizeof(path),"%s-original-seat%u-f1.ppm",output,seat);WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);}
}
static void cx4_picture(uint8_t *pixels,unsigned seat,unsigned panel,bool split);
static bool combat_effect_raster_proved;
/* Counterfactual presentation control only: untag one published particle,
 * recompose, then restore its creator and the complete canonical state. */
static bool combat_effect_raster_probe(const X3CoopWorldDraw *draw,unsigned seat,unsigned f){
  enum {PANEL=800};size_t capacity=RtlRollbackSnapshotBound(),bytes=PANEL*224u*4u;
  uint8_t *before=malloc(capacity),*after=malloc(capacity),*image=malloc(bytes),*other=malloc(bytes),*control=malloc(bytes*2);
  check(before && after && image && other && control,"charged-particle pixel control allocates bounded buffers");
  size_t n=RtlNetplaySnapshotToMemory(before,capacity);int old_seat=X3Display_PresentationSeat();bool old_split=X3Display_LocalSplit();
  unsigned old_extra=g_ppu->extraLeftRight,old_left=g_ppu->extraLeftCur,old_right=g_ppu->extraRightCur;
  g_ppu->extraLeftRight=g_ppu->extraLeftCur=g_ppu->extraRightCur=0;
  cx4_picture(image,seat,PANEL,false);X3CoopWorldDraw *temporary=(X3CoopWorldDraw *)draw;uint8_t owner=temporary->owner;
  temporary->owner=0;cx4_picture(control,seat,PANEL,false);temporary->owner=owner;
  unsigned differences=0;for(unsigned i=0;i<PANEL*224;++i)differences+=memcmp(image+i*4,control+i*4,3)!=0;
  const char *out=getenv("X3_COOP_EFFECT_OUTPUT");if(differences && out && *out){char path[1024];snprintf(path,sizeof(path),"%s-owned-f%u.ppm",out,f);WritePpm(path,image,PANEL*4,PANEL,224,0,false);snprintf(path,sizeof(path),"%s-shared-source-control-f%u.ppm",out,f);WritePpm(path,control,PANEL*4,PANEL,224,0,false);}
  cx4_picture(control,seat,PANEL,false);check(!memcmp(image,control,bytes),"restoring the creator restores every charged-particle view pixel");
  if(differences){
    cx4_picture(other,seat^1,PANEL,false);cx4_picture(control,2,PANEL,true);
    for(unsigned y=0;y<224;++y){check(!memcmp(control+(y*PANEL*2+seat*PANEL)*4,image+y*PANEL*4,PANEL*4),"charged-particle Split half matches its actual owning selected view");check(!memcmp(control+(y*PANEL*2+(seat^1)*PANEL)*4,other+y*PANEL*4,PANEL*4),"the partner's charged-particle Split half matches its own actual selected view");}
    cx4_picture(control,2,256,false);check(X3Display_RenderedPrivateFrame(),"native-width Shared actually uses the independent creator payloads");
    fprintf(stderr,"native EFFECT RASTER seat%u f%u actor%04x visibleChangedPixels%u bothViews/Split/Shared exact\n",seat,f,draw->address,differences);
  }
  X3Display_SetPresentationSeat(old_seat);X3Display_SetLocalSplit(old_split);X3Display_SetCoopPanelWidth(256);
  g_ppu->extraLeftRight=old_extra;g_ppu->extraLeftCur=old_left;g_ppu->extraRightCur=old_right;
  size_t a=RtlNetplaySnapshotToMemory(after,capacity);same(before,RtlNetplaySnapshotHashedBytes(n),after,RtlNetplaySnapshotHashedBytes(a),"all charged-particle presentation controls leave the entire canonical simulation unchanged");
  free(before);free(after);free(image);free(other);free(control);return differences!=0;
}
static void combat_effect_match(unsigned seat,unsigned f){
  if(!getenv("X3_COOP_COMBAT_EFFECT_VISUAL"))return;
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();const CombatEffectPicture *native=&combat_effect_pictures[seat][f];
  for(unsigned d=0;d<visible->count;++d){const X3CoopWorldDraw *draw=&visible->actors[d];const uint8_t *b=draw->body;
    if(draw->owner!=seat+1 || draw->address==X3_COOP_BODY)continue;
    const uint8_t *rom=combat_rom(0x8d8000+b[0x16]*3,3);unsigned at=rom[0]|rom[1]<<8|rom[2]<<16;
    rom=combat_rom(at+(b[0x17]&127)*3,3);at=rom[0]|rom[1]<<8|rom[2]<<16;unsigned count=*combat_rom(at,1);
    check(count<=64,"actual owned effect uses a bounded original sprite layout");rom=combat_rom(at+1,count*4);
    for(unsigned piece=0;piece<count;++piece){const uint8_t *cell=rom+piece*4;unsigned side=cell[0]&32?2:1,attributes=(cell[0]&0xdf)|(b[0x11]&63),tile=(cell[3]+b[0x18])&255;
      unsigned base=(attributes&1?PPU_objTileAdr2(g_ppu):PPU_objTileAdr1(g_ppu))+tile*16;
      for(unsigned ty=0;ty<side;++ty)for(unsigned tx=0;tx<side;++tx){unsigned adr=(base+ty*256+tx*16)&32767;
        const uint8_t *owned=X3CoopPresentationObjectTile(draw,adr,g_snes->cart->rom,g_snes->cart->romImageSize);
        const uint8_t *actual_tile=owned?owned:(const uint8_t *)(g_ppu->vram+adr);
        bool tile_equal=true;for(unsigned w=0;w<16;++w){unsigned actual_word=owned?(owned[w*2]|owned[w*2+1]<<8):g_ppu->vram[adr+w];tile_equal&=native->vram[adr+w]==actual_word;}
        if(!tile_equal){
          fprintf(stderr,"native FX TILE original/shared/resolved: ");for(unsigned source=0;source<3;++source){const uint8_t *bytes=source==0?(const uint8_t *)(native->vram+adr):source==1?(const uint8_t *)(g_ppu->vram+adr):actual_tile;fprintf(stderr," [%u]",source);if(bytes)for(unsigned byte=0;byte<32;++byte)fprintf(stderr,"%02x",bytes[byte]);}fputc('\n',stderr);
          const char *output=getenv("X3_COOP_EFFECT_OUTPUT");if(output && *output){char path[1024];snprintf(path,sizeof(path),"%s-paired-f%u.ppm",output,f);WritePpm(path,g_ppu->renderBuffer,g_ppu->renderPitch,256,224,0,false);snprintf(path,sizeof(path),"%s-paired-f%u.sav",output,f);check(RtlSaveSnapshot(path),"actual native effect mismatch preserves its unchanged field");}
          fprintf(stderr,"native FX CHR seat%u f%u actor%04x class%02x group%02x pose%02x tile%04x\n",seat,f,draw->address,b[10],b[0x16],b[0x17]&127,adr);check(0,"every displayed owned effect tile retains its original creator's native graphics during concurrent uploads");}
        if(!combat_effect_raster_proved && getenv("X3_COOP_COMBAT_COMBO_ONLY") && owned && memcmp(native->vram+adr,g_ppu->vram+adr,32))combat_effect_raster_proved=combat_effect_raster_probe(draw,seat,f);
        ++combat_effect_tiles[seat];
        for(unsigned y=0;y<8;++y)for(unsigned x=0;x<8;++x){unsigned lo=native->vram[adr+y],hi=native->vram[adr+8+y],shift=7-x;
          unsigned ink=((lo>>shift)&1)|(((lo>>(8+shift))&1)<<1)|(((hi>>shift)&1)<<2)|(((hi>>(8+shift))&1)<<3);if(!ink)continue;
          unsigned index=((attributes>>1)&7)*16+ink;const uint8_t *own=visible->players.players[seat].palette;
          unsigned actual=index<32?(own[index*2]|own[index*2+1]<<8):g_ppu->cgram[128+index];
          if((actual&32767)!=(native->colors[index]&32767)){fprintf(stderr,"native FX COLOR seat%u f%u actor%04x class%02x group%02x index%u original%04x paired%04x\n",seat,f,draw->address,b[10],b[0x16],index,native->colors[index],actual);check(0,"every opaque owned effect pixel retains its original creator's native colors during concurrent uploads");}
          ++combat_effect_ink[seat];
        }
      }
    }
  }
}
static void combat_visual_finish(void){
  if(getenv("X3_COOP_COMBAT_EFFECT_VISUAL")){
    check(combat_effect_tiles[0] && combat_effect_tiles[1] && combat_effect_ink[0] && combat_effect_ink[1],"both creators actually display nonempty native owned effect art");
    fprintf(stderr,"native EFFECT VISUAL tiles%u/%u opaquePixels%u/%u alloriginalCHR/CGRAMexact\n",combat_effect_tiles[0],combat_effect_tiles[1],combat_effect_ink[0],combat_effect_ink[1]);
    if(getenv("X3_COOP_COMBAT_COMBO_ONLY"))check(combat_effect_raster_proved,"native creator payloads change real visible particle pixels, not only unused tiles");
    combat_effect_raster_proved=false;
    for(unsigned seat=0;seat<2;++seat){free(combat_effect_pictures[seat]);combat_effect_pictures[seat]=NULL;combat_effect_tiles[seat]=combat_effect_ink[seat]=0;}
  }
}
static void combat_picture_capture(unsigned seat,unsigned f){
  combat_effect_capture(seat,f);CombatPicture *p=&combat_pictures[seat][f];memcpy(p->chr,g_ppu->vram+0x6000,sizeof(p->chr));memcpy(p->palette,g_ppu->cgram+128,sizeof(p->palette));
  if(f==0 && getenv("X3_COOP_WEAPON_STATE") && getenv("X3_COOP_EFFECT_OUTPUT")){char name[100];snprintf(name,sizeof(name),"weapon-original-seat%u-f0.sav",seat);check(RtlSaveSnapshot(name),"original weapon graphics preserves its real initial native field");}
}
static void combat_picture_match(unsigned seat,unsigned f){
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();const uint8_t *b=visible->players.players[seat].body,*atlas=visible->body_chr[seat];
  const CombatPicture *native=&combat_pictures[seat][f];
  const uint8_t *p=combat_rom(0x8d8000+b[0x16]*3,3);unsigned at=p[0]|p[1]<<8|p[2]<<16;
  p=combat_rom(at+(b[0x17]&127)*3,3);at=p[0]|p[1]<<8|p[2]<<16;p=combat_rom(at,1);unsigned pieces=*p;
  check(pieces && pieces<=64,"actual concurrent pose uses an original nonempty native sprite layout");p=combat_rom(at,1+pieces*4);
  for(unsigned i=0;i<pieces;++i){const uint8_t *piece=p+1+i*4;unsigned size=piece[0]&32?2:1,tile=piece[3];
    for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x){unsigned index=(((tile>>4)+y)&15)*16+((tile+x)&15),offset=index*32;
      if(memcmp(atlas+offset,native->chr+offset,32)){fprintf(stderr,"native BODY CHR mismatch seat%u f%u group%u pose%u tile%u\n",seat,f,b[0x16],b[0x17]&127,index);if(getenv("X3_COOP_EFFECT_OUTPUT"))check(RtlSaveSnapshot("weapon-actual-graphics-unqualified.sav"),"mismatched body graphics retains its unchanged native field");check(0,"every displayed native body tile matches actual original VRAM during concurrent movement and attacks");}}
  }
  const uint8_t *colors=visible->players.players[seat].palette;
  for(unsigned i=0;i<32;++i)if(((colors[i*2]|colors[i*2+1]<<8)&32767)!=((native->palette[i*2]|native->palette[i*2+1]<<8)&32767)){
    fprintf(stderr,"native BODY PALETTE mismatch seat%u f%u color%u original%04x paired%04x\n",seat,f,i,native->palette[i*2]|native->palette[i*2+1]<<8,colors[i*2]|colors[i*2+1]<<8);
    check(0,"displayed native character colors match original CGRAM on each actual concurrent field");
  }
  const PpuExtraObject *body=X3CoopRenderPlayer(seat,0,0);bool opaque=false;
  /* The protected weapon fixture retains native damage blinking. A hidden
   * published body still needs nonempty, unchanged character art: expose only
   * its host visibility tag for this raster control, then restore it. */
  if(!b[14]){uint8_t *show=(uint8_t *)b;show[14]=1;body=X3CoopRenderPlayer(seat,0,0);show[14]=0;}
  if(body)for(unsigned i=0;i<128*128;++i)opaque|=body->pixels[i]!=0;
  check(opaque,"the original-size native body raster remains visible during concurrent movement and attacks");
  combat_effect_match(seat,f);
  const char *out=getenv("X3_COOP_EFFECT_OUTPUT");if(out && *out && getenv("X3_COOP_COMBAT_COMBO_ONLY") && f==243 && seat==0){char path[1024];snprintf(path,sizeof(path),"%s-live-roster%u-f243.sav",out,visible->players.players[0].character);check(RtlSaveSnapshot(path),"actual live charged particles save for their affected fade regression");}
}
static void combat_frame(unsigned input){
  unsigned before=g_ram[0x9d3];frame(input);memset(&combat_cues,0,sizeof(combat_cues));combat_hit_cues=0;
  if(!combat_audio_enabled)return;
  unsigned end=g_ram[0x9d3];check(before<32 && end<32 && !(before&1) && !(end&1),"native sound queue retains its original aligned request indices");
  for(unsigned at=before;at!=end;at=(at+2)&31){unsigned i=combat_cues.count++;combat_cues.ids[i]=g_ram[0x9a2+at];combat_cues.pan[i]=g_ram[0x9a3+at];if(combat_cues.ids[i]==0x20)++combat_hit_cues;}
}
static void combat_audio_match(unsigned f){
  const CombatCues *a=&combat_reference_cues[0][f],*b=&combat_reference_cues[1][f];bool used[32]={0};
  if(a->count+b->count!=combat_cues.count)fprintf(stderr,"native AUDIO MISMATCH f%u original%u+%u paired%u\n",f,a->count,b->count,combat_cues.count);
  check(a->count+b->count==combat_cues.count,"simultaneous native attacks queue exactly their two original per-field sound request counts");
  for(unsigned seat=0;seat<2;++seat){const CombatCues *c=seat?b:a;for(unsigned i=0;i<c->count;++i){unsigned match=0;while(match<combat_cues.count && (used[match] || c->ids[i]!=combat_cues.ids[match] || c->pan[i]!=combat_cues.pan[match]))++match;check(match<combat_cues.count,"native concurrent cue IDs and spatial payloads retain both original actor cadences");used[match]=true;}}
}
typedef struct CombatSample {uint8_t command,charge[2],busy[2],controls[12],pose[6],palette[64],aux[X3_COOP_AUX_BYTES],shots[X3_COOP_SHOT_BYTES],hit[3];} CombatSample;

static unsigned combat_attack_buttons(unsigned mode,unsigned seat,unsigned f){
  if(mode==4)return f<216 || f%4==0?SNES_PAD_Y:0;
  if(!mode)return f%4==0?SNES_PAD_Y:0;
  if(mode==1)return f<(seat?216:180)?SNES_PAD_Y:0;
  return seat==(mode==2?0:1)?(f<180?SNES_PAD_Y:0):(f%4==0?SNES_PAD_Y:0);
}
static unsigned combat_buttons(unsigned mode,unsigned seat,unsigned f){
  unsigned keys=combat_attack_buttons(mode,seat,f);
  if(combat_visual_enabled){if(f<48)keys|=((f<24)^(mode==4 && !getenv("X3_COOP_COMBAT_OPPOSED")?1:seat))?SNES_PAD_RIGHT:SNES_PAD_LEFT;if(f>=48 && f<72)keys|=SNES_PAD_B;}
  return keys;
}
static void combat_read(CombatSample *out,unsigned seat,bool original){
  memset(out,0,sizeof(*out));
  const X3CoopState *s=X3CoopRuntimeState();const X3CoopPlayer *p=&s->players[seat];bool live=original || s->current==seat;
  const uint8_t *body=live?g_ram+X3_COOP_BODY:p->body;out->command=live?g_ram[0x1f1e]:p->shot_command;
  out->pose[0]=body[2];out->pose[1]=body[3];/*04:BCAF consumes the native DMA-request bit; the private atlas consumes the same pose without that shared-VRAM write. */out->pose[2]=body[0x17]&127;out->pose[3]=body[0x59];out->pose[4]=body[0x16];out->pose[5]=body[0x11];out->busy[0]=body[0x8f];out->busy[1]=body[0xb7];const unsigned control_offsets[12]={0xac,0x59,0x6e,0x3b,0x33,0x7d,0xaa,0x8e,0x36,0x37,0x38,0x39};for(unsigned i=0;i<12;++i)out->controls[i]=body[control_offsets[i]];memcpy(out->charge,body+0x57,2);memcpy(out->palette,live?g_ram+X3_COOP_PALETTE:p->palette,64);
  memcpy(out->aux,live?g_ram+X3_COOP_AUX:p->auxiliaries,X3_COOP_AUX_BYTES);memcpy(out->shots,live?g_ram+X3_COOP_SHOTS:p->shots,X3_COOP_SHOT_BYTES);
}
static void combat_probe(CombatSample *out,unsigned seat,bool original,uint8_t *scratch,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);check(n!=0,"native weapon hit-window snapshot saves");
  if(!original){X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;door_install_player(&solo);}
  X3CoopRuntimeEnable(false);
  unsigned x=view_read(X3_COOP_BODY+5),y=view_read(X3_COOP_BODY+8);
  contact_geometry(0xd18,x+32,y-8);g_ram[0xd18+0x28]=0;g_ram[0xd18+0x27]=127;
  uint16_t stack=g_cpu.S;g_cpu.PB=4;g_cpu.DB=6;g_cpu.D=0xd18;g_cpu.P=(uint8_t)((g_cpu.P|0x30)&~0x40);cpu_p_to_mirrors(&g_cpu);cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,0x04cb74) && g_cpu.S==stack,"actual native weapon hit-window call balances");
  out->hit[0]=g_ram[0xd18+0x27];out->hit[1]=(uint8_t)g_cpu.A;out->hit[2]=g_ram[0xd18+0x30];
  check(RtlLoadSnapshotFromMemory(scratch,n),"actual weapon hit-window probe restores its unchanged combat timeline");
  if(original)X3CoopRuntimeEnable(false);
}
/* Shared retail allocation preserves a freed slot's fractional coordinates.
 * A separate solo timeline can choose a different slot. For that one counter
 * only, qualify the complete original next field from the actual prior world;
 * keep the long native command, charge, pose, hit-window and pool comparisons. */
static bool combat_shared_history_match(const CombatSample *reference,const CombatSample *actual,
    unsigned seat,unsigned mode,unsigned f,const uint8_t *prior,size_t prior_size,
    uint8_t *post,uint8_t *scratch,size_t capacity){
  if(!getenv("X3_COOP_COMBAT_SHARED_HISTORY_REFERENCE") || mode!=4)return false;
  CombatSample adjusted=*reference;adjusted.controls[6]=actual->controls[6];
  if(!memcmp(reference,actual,sizeof(adjusted)) || memcmp(&adjusted,actual,sizeof(adjusted)))return false;
  size_t after=RtlSaveSnapshotToMemory(post,capacity),owners=combat_owners(prior,prior_size);
  check(RtlLoadSnapshotFromMemory(prior,prior_size),"shared child reference restores the actual unchanged prior world");
  X3CoopState solo=*X3CoopRuntimeState();unsigned before=solo.players[seat].body[0xaa],children=0;
  uint8_t native_effects[1280];
  for(unsigned i=0;i<40;++i)if(prior[owners+i]){
    check(prior[owners+i]==seat+1,"focused native child reference has no other creator's live shared effects");
    check(g_ram[0x1818+32*i+10]==0x34,"counter variation belongs to the original class34 child family");++children;
  }
  check(children && before,"the differing count has actual original child records before their update");
  solo.current=(uint8_t)seat;memcpy(scratch,prior,prior_size);
  check(X3CoopEncode(&solo,scratch+contact_context_offset(scratch,prior_size),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(scratch,prior_size),"original next-field reference selects the actual creator and all its unchanged pools");
  door_install_player(&solo);X3CoopViewState view=*X3CoopViewsState();vehicle_install_view(&view,seat);X3CoopRuntimeEnable(false);
  combat_frame(combat_buttons(mode,seat,f));CombatSample native;combat_read(&native,seat,true);
  if(solo.players[seat].character==X3_COOP_ZERO)combat_probe(&native,seat,true,scratch,capacity);
  memcpy(native_effects,g_ram+0x1818,sizeof(native_effects));
  bool exact=!memcmp(&native,actual,sizeof(native));
  check(RtlLoadSnapshotFromMemory(post,after),"native shared-history reference restores the complete actual paired field");
  for(unsigned i=0;i<40;++i)if(prior[owners+i]==seat+1)
    check(!memcmp(native_effects+32*i,g_ram+0x1818+32*i,32),"actual shared child's entire original movement, animation and retirement record matches one native field");
  check(exact,"actual shared child counter, command, charge, pose, hit window and every owned pool match the original next field");
  fprintf(stderr,"native SHARED CHILD HISTORY seat%u f%u priorAA%u originalNext%u paired%u separateSolo%u children%u exactNativeRecords\n",seat,f,before,native.controls[6],actual->controls[6],reference->controls[6],children);
  return true;
}
static void combat_retirement(const uint8_t *base,size_t n,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned phase=0;phase<4;++phase)for(unsigned victim=0;victim<2;++victim){
    check(RtlLoadSnapshotFromMemory(base,n),"combat retirement restores the neutral actual roster");X3CoopState setup=*X3CoopRuntimeState();
    if(phase==3 && setup.players[victim].character!=X3_COOP_X)continue;
    for(unsigned seat=0;seat<2;++seat){uint8_t *b=setup.players[seat].body;unsigned x=384+128*seat;b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);}
    if(phase<2)setup.players[victim].body[0x27]=2;memcpy(work,base,n);
    check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"separated actors and low victim HP establish a fatal-contact prerequisite");door_install_player(&setup);
    unsigned frames=phase==1 || phase==3?227:phase==2?216:200;for(unsigned f=0;f<frames;++f)frame(phase==3?combat_buttons(4,victim,f)<<(victim*12):combat_buttons(4,0,f)|(combat_buttons(4,1,f)<<12));
    size_t before=RtlSaveSnapshotToMemory(work,capacity);X3CoopState live=*X3CoopRuntimeState();unsigned survivor=victim^1;
    check(live.players[victim].life==X3_COOP_ALIVE && live.players[survivor].life==X3_COOP_ALIVE,"native charge/beam death starts with two living actors");
    if(phase==0 || phase==2)check(live.players[victim].body[0x58]>=4 && live.players[survivor].body[0x58]>=4,"both actors actually charge before fatal damage");
    size_t owner_at=door_scene_chunk(work,before)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;unsigned owners[2]={0,0};
    for(unsigned i=0;i<40;++i)if(work[owner_at+i])++owners[work[owner_at+i]-1];
    if(phase==1)check(owners[0] && owners[1],"both real charged attacks retain shared native child effects before fatal contact");
    if(phase>=2){
      if(phase==3)check(live.players[victim].body[0x7d] || live.players[victim].body[0xaa],"actual charged X beam/children own nonzero native attack counters before withdrawal");
      if(phase==2 && live.players[victim].character==X3_COOP_ZERO)check(live.players[victim].body[0xb7]!=0,"Zero actually arms the native combo before living withdrawal");
      check(X3CoopRuntimeWithdraw(victim),"living actor withdraws with native charge armed");const X3CoopPlayer *p=&X3CoopRuntimeState()->players[victim];
      fprintf(stderr,"withdraw charge victim%u char%u charge%u/%u combo%u AA%u count%u\n",victim,p->character,p->body[0x57],p->body[0x58],p->body[0xb7],p->body[0xaa],p->body[0x7d]);
      check(!p->body[0x35] && !p->body[0x57] && !p->body[0x58] && !p->body[0xb7] && !p->body[0xaa] && !p->body[0x7d],"withdrawal cancels the actual charge/combo and all retired native shot counters");
      check(X3CoopRuntimeRejoin(victim),"charged withdrawal returns on real clear native ground");frame(0);
      p=&X3CoopRuntimeState()->players[victim];check(p->life==X3_COOP_ALIVE && p->body[2]!=0x5c && !p->body[0xb7],"return requires the real native charge/combo again before a saber swing");
      bool fired=false;unsigned firing=0;for(;firing<40 && !fired;++firing){frame((firing%4==0?SNES_PAD_Y:0)<<(victim*12));p=&X3CoopRuntimeState()->players[victim];for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)fired|=p->shots[i]!=0;}fprintf(stderr,"returned native fire phase%u victim%u frames%u recoil%u busy%u enabled%u stock%u world%u\n",phase,victim,firing,p->body[0x59],p->body[0x8f],X3CoopRuntimeEnabled(),X3CoopRuntimeStockZero(),g_ram[0x9cc]);if(!fired){fprintf(stderr,"blocked returned attack char%u state%02x/%02x input%04x/%04x/%04x selected%u hold%u charge%u/%u gate7c%u gate7d%u AA%u B7%u capture%u script%02x global%u\n",p->character,p->body[2],p->body[3],p->body[0x36]|p->body[0x37]<<8,p->body[0x38]|p->body[0x39]<<8,p->body[0x3a]|p->body[0x3b]<<8,p->body[0x33],p->body[0x59],p->body[0x57],p->body[0x58],p->body[0x7c],p->body[0x7d],p->body[0xaa],p->body[0xb7],p->body[0x6e],p->body[0xa3],g_ram[0x1f36]);check(RtlSaveSnapshot("return-fire-failed.sav"),"blocked native return preserves a disposable diagnostic snapshot");}check(fired,"returned actor fires a fresh real native buster without stale allocation locks");
      continue;
    }
    contact_geometry(0xd18,support_position(victim,5),support_position(victim,8));CpuState caller=g_cpu;contact_native_call(0xd18,0x04cb31);
    /* This test injects a call between host frames. Retain its elapsed guest
     * time while restoring the outer frame caller, especially its direct page. */
    uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;
    const X3CoopState *s=X3CoopRuntimeState();const X3CoopPlayer *p=&s->players[victim],*other=&s->players[survivor],*old=&live.players[survivor];
    check(p->life==X3_COOP_FALLEN && other->life==X3_COOP_ALIVE && !s->wipe && s->anchor==survivor,"actual native enemy damage retires only the charging/firing victim");
    bool empty=true;for(unsigned i=0;i<sizeof(p->shots);++i)empty&=!p->shots[i];for(unsigned i=0;i<sizeof(p->auxiliaries);++i)empty&=!p->auxiliaries[i];
    check(empty && !p->shot_command,"fatal attack retirement removes all victim-owned native damage and charge sources");
    check(other->body[0x27]==old->body[0x27] && !memcmp(other->body+0x57,old->body+0x57,2) && !memcmp(other->shots,old->shots,sizeof(other->shots)) && !memcmp(other->auxiliaries,old->auxiliaries,sizeof(other->auxiliaries)) && !memcmp(other->palette,old->palette,sizeof(other->palette)),"survivor retains its exact health, native charge, attacks, effects and palette at fatal handoff");
    size_t end=RtlSaveSnapshotToMemory(after,capacity);unsigned retained=0;owner_at=door_scene_chunk(after,end)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;
    for(unsigned i=0;i<40;++i){check(after[owner_at+i]!=victim+1,"dead actor retains no shared native child-effect owner");retained+=after[owner_at+i]==survivor+1;}
    check(retained==owners[survivor],"fatal damage preserves every surviving shared child-effect allocation");
    unsigned tick=g_ram[0x9cc];for(unsigned f=0;f<24;++f)frame((SNES_PAD_RIGHT|SNES_PAD_Y)<<(survivor*12));
    fprintf(stderr,"retirement continuation phase%u victim%u tick%u->%u states%u/%u hp%u/%u display%02x tasks%u/%u freeze%02x/%02x/%02x/%02x/%02x/%02x/%02x position%u,%u\n",phase,victim,tick,g_ram[0x9cc],X3CoopRuntimeState()->players[victim].life,X3CoopRuntimeState()->players[survivor].life,contact_hp(victim),contact_hp(survivor),g_ppu->inidisp,g_ram[0xd1],g_ram[0xd2],g_ram[0x1f25],g_ram[0x1f26],g_ram[0x1f27],g_ram[0x1f28],g_ram[0x1f29],g_ram[0x1f2a],g_ram[0x1f2b],support_position(survivor,5),support_position(survivor,8));
    check((uint8_t)(g_ram[0x9cc]-tick)==24 && X3CoopRuntimeState()->players[survivor].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[victim].life==X3_COOP_FALLEN && !(g_ppu->inidisp&0x80),"survivor continues actual movement/combat and one bright shared world after charged death");
    fprintf(stderr,"native combat retirement phase%u victim%u char%u ownerChildren%u/%u survivorCharge%u world24\n",phase,victim,p->character,owners[0],owners[1],old->body[0x58]);
  }
}
static void combat_select_creator(unsigned seat,uint8_t *scratch,size_t capacity){
  size_t n=RtlSaveSnapshotToMemory(scratch,capacity);X3CoopState s=*X3CoopRuntimeState();s.current=(uint8_t)seat;
  check(X3CoopEncode(&s,scratch+contact_context_offset(scratch,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(scratch,n),"native effect allocator selects its real creator context");door_install_player(&s);
}
static unsigned combat_allocate_effect(unsigned pc,unsigned d){
  CpuState caller=g_cpu;g_cpu.PB=2;g_cpu.DB=0;g_cpu.D=(uint16_t)d;g_cpu.A=0;g_ram[0]=g_ram[1]=0;g_cpu.P=(uint8_t)((g_cpu.P|0x20)&~0x10);cpu_p_to_mirrors(&g_cpu);cpu_push_jsl_return_frame(&g_cpu);
  check(interp_bridge_run(&g_cpu,pc) && g_cpu.S==caller.S,"real shared-effect constructor/allocator returns with its balanced guest caller");
  unsigned slot=g_cpu.X;uint64_t elapsed=g_cpu.master_cycles;uint32_t cycles=g_cpu.cycles;g_cpu=caller;g_cpu.master_cycles=elapsed;g_cpu.cycles=cycles;return slot;
}
static size_t combat_owners(const uint8_t *bytes,size_t n){return door_scene_chunk(bytes,n)+16+8+X3_COOP_VIEW_STATE_BYTES+67+114;}
static void combat_effect_pressure(uint8_t *base,uint8_t *work,uint8_t *after,size_t capacity){
  for(unsigned f=0;f<12;++f)frame((f%4==0?SNES_PAD_Y:0)*0x1001u);
  size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState live=*X3CoopRuntimeState();
  for(unsigned seat=0;seat<2;++seat){unsigned shots=0;for(unsigned i=0;i<X3_COOP_SHOT_BYTES;i+=64)shots+=live.players[seat].shots[i]!=0;check(shots>=2,"both native projectile pools contain real live busters during effect pressure");}
  unsigned created[2]={0,0},used=0;for(unsigned i=0;i<40;++i)used+=g_ram[0x1818+i*32]!=0;
  for(unsigned attempt=0;used<40 && attempt<40;++attempt){unsigned seat=attempt&1;combat_select_creator(seat,work,capacity);unsigned slot=combat_allocate_effect(0x02d9a8,X3_COOP_BODY);
    if(slot<0x1818 || slot>=0x1d18 || g_ram[slot+10]!=0x37)fprintf(stderr,"native effect construct seat%u slot%04x bytes%02x/%02x/%02x/%02x AA%u\n",seat,slot,g_ram[slot&0x1ffff],g_ram[(slot+1)&0x1ffff],g_ram[(slot+10)&0x1ffff],g_ram[(slot+11)&0x1ffff],g_ram[X3_COOP_BODY+0xaa]);
    check(slot>=0x1818 && slot<0x1d18 && !((slot-0x1818)%32) && g_ram[slot] && g_ram[slot+10]==0x37,"actual native weapon-child constructor fills a free shared slot");
    size_t size=RtlSaveSnapshotToMemory(after,capacity),owners=combat_owners(after,size);check(after[owners+(slot-0x1818)/32]==seat+1,"allocated native child retains the actual creator rather than the world anchor");++used;++created[seat];}
  check(used==40 && created[0] && created[1],"competing real creators fill all40 native shared-effect slots");
  n=RtlSaveSnapshotToMemory(base,capacity);size_t owners=combat_owners(base,n);uint8_t records[1280];memcpy(records,g_ram+0x1818,sizeof(records));
  check(RtlLoadSnapshotFromMemory(base,n),"full native effect pressure snapshot restores");same(base,n,after,RtlSaveSnapshotToMemory(after,capacity),"full native child ownership and the complete machine restore exactly");
  for(unsigned seat=0;seat<2;++seat){combat_select_creator(seat,work,capacity);unsigned aa=X3CoopRuntimeState()->players[seat].body[0xaa];unsigned slot=combat_allocate_effect(0x02d9a8,X3_COOP_BODY);
    check(slot==0x1d18 && !memcmp(records,g_ram+0x1818,sizeof(records)),"full retail allocator refuses another child without replacing any live effect");
    size_t size=RtlSaveSnapshotToMemory(after,capacity),at=combat_owners(after,size);check(!memcmp(base+owners,after+at,40) && X3CoopRuntimeState()->players[seat].body[0xaa]==aa,"failed native allocation preserves both owners and the creator's count");
    for(unsigned s=0;s<2;++s)check(!memcmp(live.players[s].shots,X3CoopRuntimeState()->players[s].shots,X3_COOP_SHOT_BYTES),"effect-pool exhaustion preserves each complete live projectile and hit-history pool");}
  combat_select_creator(0,work,capacity);check(X3CoopRuntimeWithdraw(0),"one living creator withdraws while both pools are pressured");
  size_t size=RtlSaveSnapshotToMemory(after,capacity),at=combat_owners(after,size);unsigned freed=0;
  for(unsigned i=0;i<40;++i){if(base[owners+i]==1){++freed;check(!g_ram[0x1818+i*32] && !after[at+i],"withdrawal releases only its creator's actual shared child slot");}
    else check(after[at+i]==base[owners+i] && !memcmp(g_ram+0x1818+i*32,records+i*32,32),"other-creator and unowned world effects retain exact native records under withdrawal");}
  check(freed && !memcmp(live.players[1].shots,X3CoopRuntimeState()->players[1].shots,X3_COOP_SHOT_BYTES),"surviving live projectiles and hit history remain exact after competing creator withdrawal");
  for(unsigned i=0;i<freed;++i){unsigned slot=combat_allocate_effect(0x02d9a8,X3_COOP_BODY);check(slot<0x1d18 && slot>=0x1818,"survivor reuses an actual freed native effect slot");}
  size=RtlSaveSnapshotToMemory(after,capacity);at=combat_owners(after,size);for(unsigned i=0;i<40;++i)if(base[owners+i]==1)check(after[at+i]==2,"reused child slots belong to their new real creator");
  /* A native allocator must also discard a former personal tag when an
   * inactive slot is reused by a world caller. Inactivity is a prerequisite,
   * not an assertion about a particular authored effect's cleanup. */
  unsigned slot=0x1818;while(slot<0x1d18 && after[at+(slot-0x1818)/32]!=2)slot+=32;check(slot<0x1d18,"world reuse has a former personal slot");memset(g_ram+slot,0,32);
  unsigned reused=combat_allocate_effect(0x02d859,0xd18);size=RtlSaveSnapshotToMemory(work,capacity);check(reused==slot && !work[combat_owners(work,size)+(slot-0x1818)/32],"unowned native allocation resets the old personal tag on actual slot reuse");
  fprintf(stderr,"native effect pressure used%u created%u/%u retired%u reusedWorld%04x\n",used,created[0],created[1],freed,reused);
}
static void native_concurrent_combat_checks(const char *normal,const char *reversed,uint8_t *base,uint8_t *work,uint8_t *future,size_t capacity){
  bool earned_saber=getenv("X3_COOP_COMBAT_EARNED_SABER")!=NULL;
  enum {FRAMES=420};CombatSample *trace=malloc(2*FRAMES*sizeof(*trace));uint8_t *probe=malloc(capacity);check(probe!=NULL,"native damage-window scratch allocates");check(trace!=NULL,"native combat comparison traces allocate");
  for(unsigned roster=0;roster<(getenv("X3_COOP_COMBAT_PREPARE_GUI")?1:2);++roster){
    check(RtlLoadSnapshot(earned_saber?normal:roster?reversed:normal) && g_ram[0x1fae]==(earned_saber?11:1),"native concurrent combat loads its actual stage roster");
    if(earned_saber)check((g_ram[0x1fb2]&0xc0)==0xc0 && !X3CoopRuntimeStockZero(),"saber combat consumes the actually earned native award with full-campaign Zero");
    size_t n=RtlSaveSnapshotToMemory(base,capacity);X3CoopState setup=*X3CoopRuntimeState();X3CoopCapture(&setup,g_ram);if(earned_saber && roster){X3CoopPlayer swap=setup.players[0];setup.players[0]=setup.players[1];setup.players[1]=swap;}setup.current=setup.anchor=(uint8_t)(combat_visual_enabled?(setup.players[0].character==X3_COOP_X?0:1):0);
    for(unsigned seat=0;seat<2;++seat){X3CoopPlayer *p=&setup.players[seat];uint8_t *b=p->body;unsigned x=earned_saber?3200:384,y=(earned_saber?1968:1200)-(p->character==X3_COOP_ZERO?25:17);
      b[5]=b[0x22]=(uint8_t)x;b[6]=b[0x23]=(uint8_t)(x>>8);b[8]=b[0x24]=(uint8_t)y;b[9]=b[0x25]=(uint8_t)(y>>8);b[2]=b[3]=b[4]=b[7]=0;memset(b+0x1a,0,6);b[0x33]=b[0x57]=b[0x58]=b[0x88]=b[0x8a]=0;memset(b+0x38,0,4);memset(p->auxiliaries,0,sizeof(p->auxiliaries));memset(p->shots,0,sizeof(p->shots));p->shot_command=0;p->previous_actions=0;}
    memcpy(work,base,n);check(X3CoopEncode(&setup,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"combat position, neutral weapons and empty owned pools restore");door_install_player(&setup);X3CoopViewsConfigure(combat_visual_enabled);if(combat_visual_enabled)X3CoopViewsReset();
    memset(g_ram+0xd18,0,0x3c0);memset(g_ram+0x1318,0,0x500);
    for(unsigned col=0;col<255;++col){unsigned first=view_read(0xf800+2*col),end=view_read(0xf802+2*col);if(first<0xfa02 || end>0xfffc || end<first || (end-first)%5)continue;for(unsigned at=first;at<end;at+=5)g_ram[at]=1;}
    view_word(0x1e5d,earned_saber?3072:256);view_word(0x1e7a,earned_saber?3072:256);const unsigned vertical[]={0x1e60,0x1e7c,0x1e6a,0x1e6c,0x1e78,0x1e7e};for(unsigned i=0;i<6;++i)view_word(vertical[i],earned_saber?1792:1024);
    for(unsigned i=0;i<12;++i)combat_frame(0);check(X3CoopRuntimeState()->players[0].life==X3_COOP_ALIVE && X3CoopRuntimeState()->players[1].life==X3_COOP_ALIVE,"both native combat actors settle on actual stage1 ground");
    n=RtlSaveSnapshotToMemory(base,capacity);
    if(getenv("X3_COOP_COMBAT_HURT")){
      unsigned zero=setup.players[0].character==X3_COOP_ZERO?0:1;const unsigned cuts[3]={200,228,305};
      for(unsigned phase=0;phase<3;++phase){check(RtlLoadSnapshotFromMemory(base,n),"Zero nonfatal hurt restores the same quiet native stage");for(unsigned f=0;f<cuts[phase];++f)combat_frame((uint32_t)combat_buttons(4,zero,f)<<(zero*12));size_t cut=RtlSaveSnapshotToMemory(work,capacity);native_weapon_hurt_lifetime(work,cut,zero,0,phase,future,probe,capacity);fprintf(stderr,"native Zero hurt PASS roster%u phase%u charged/combo/saberCut%u\n",roster,phase,cuts[phase]);}
      continue;
    }
    if(getenv("X3_COOP_COMBAT_PREPARE_GUI")){
      X3CoopState visual=*X3CoopRuntimeState();visual.players[1].body[5]=visual.players[1].body[0x22]=0;visual.players[1].body[6]=visual.players[1].body[0x23]=2;
      memcpy(work,base,n);check(X3CoopEncode(&visual,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"visual native combat positions restore");door_install_player(&visual);X3CoopViewsConfigure(true);for(unsigned f=0;f<12;++f)combat_frame(0);
      check(RtlSaveSnapshot(getenv("X3_COOP_COMBAT_PREPARE_GUI")),"current native two-view combat fixture saves through the test interface");continue;
    }
    for(unsigned mode=getenv("X3_COOP_COMBAT_COMBO_ONLY")?4:0;mode<((getenv("X3_COOP_COMBAT_RETIREMENT_ONLY") || getenv("X3_COOP_COMBAT_POOL_ONLY"))?0:getenv("X3_COOP_COMBAT_COMBO_ONLY")?5:4);++mode){
      unsigned frames=mode==4?420:240;
      uint32_t native_spc[2]={0,0},spc_start=0;unsigned peaks[2]={0,0},maxcharge[2]={0,0},seat_saber_hits[2]={0,0},saber_hits=0,saber_first=FRAMES,saber_last=0;uint64_t actions[2]={0,0},kinds[2]={0,0};
      for(unsigned seat=0;seat<2;++seat){check(RtlLoadSnapshotFromMemory(base,n),"original native firing/charge reference restores");X3CoopState solo=*X3CoopRuntimeState();solo.current=(uint8_t)seat;memcpy(work,base,n);check(X3CoopEncode(&solo,work+contact_context_offset(work,n),X3CoopEncodedSize()) && RtlLoadSnapshotFromMemory(work,n),"original native reference selects its actual snapshot actor");door_install_player(&solo);X3CoopRuntimeEnable(false);
        /* Install inherited native character art before the original DMA stream. */
        if(combat_visual_enabled){X3CoopViewState view=*X3CoopViewsState();vehicle_install_view(&view,seat);combat_seed_body(seat,solo.players[seat].body);memcpy(g_ppu->cgram+128,solo.players[seat].palette,64);++g_ppu->vramWriteCount;}spc_start=g_snes->apu->cycles;
        for(unsigned f=0;f<frames;++f){combat_frame(combat_buttons(mode,seat,f));if(combat_visual_enabled)combat_picture_capture(seat,f);if(combat_audio_enabled){combat_reference_cues[seat][f]=combat_cues;combat_reference_camera[seat][f][0]=(uint16_t)view_read(0x1e5d);combat_reference_camera[seat][f][1]=(uint16_t)view_read(0x1e60);for(unsigned k=0;k<4;++k)combat_reference_camera[seat][f][k+2]=(uint16_t)view_read(0x1e66+k*2);}CombatSample *sample=&trace[seat*FRAMES+f];combat_read(sample,seat,true);if(mode==4 && f>=216 && (earned_saber || solo.players[seat].character==X3_COOP_ZERO)){combat_probe(sample,seat,true,probe,capacity);if(sample->pose[0]==0x5c && (sample->hit[0]&127)<127){++saber_hits;++seat_saber_hits[seat];if(f<saber_first)saber_first=f;saber_last=f;}}if(sample->pose[0]<128)actions[seat]|=UINT64_C(1)<<(sample->pose[0]/2);for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)if(sample->shots[j] && sample->shots[j+10]<64)kinds[seat]|=UINT64_C(1)<<sample->shots[j+10];unsigned count=0;for(unsigned j=0;j<X3_COOP_SHOT_BYTES;j+=64)count+=sample->shots[j]!=0;if(count>peaks[seat])peaks[seat]=count;if(sample->charge[1]>maxcharge[seat])maxcharge[seat]=sample->charge[1];}native_spc[seat]=g_snes->apu->cycles-spc_start;}
      check(RtlLoadSnapshotFromMemory(base,n),"concurrent combat restores the matched original prerequisites");unsigned tick=g_ram[0x9cc];spc_start=g_snes->apu->cycles;
      for(unsigned f=0;f<frames;++f){size_t prior_size=0;if(getenv("X3_COOP_COMBAT_SHARED_HISTORY_REFERENCE"))prior_size=RtlSaveSnapshotToMemory(work,capacity);combat_frame(combat_buttons(mode,0,f)|(combat_buttons(mode,1,f)<<12));if(combat_audio_enabled)combat_audio_match(f);if(combat_visual_enabled && (f==24 || f==60))for(unsigned seat=0;seat<2;++seat)check(f==24?support_position(seat,5)!=(setup.players[seat].body[5]|setup.players[seat].body[6]<<8):support_position(seat,8)+8<(setup.players[seat].body[8]|setup.players[seat].body[9]<<8),f==24?"both native characters actually move independently during the visual comparison":"both native characters actually jump while charging or firing");for(unsigned seat=0;seat<2;++seat){if(combat_visual_enabled)combat_picture_match(seat,f);CombatSample actual;combat_read(&actual,seat,false);if(mode==4 && f>=216 && (earned_saber || X3CoopRuntimeState()->players[seat].character==X3_COOP_ZERO))combat_probe(&actual,seat,false,probe,capacity);CombatSample *expected=&trace[seat*FRAMES+f];if(memcmp(expected,&actual,sizeof(actual)) && !combat_shared_history_match(expected,&actual,seat,mode,f,work,prior_size,future,probe,capacity)){unsigned at=0;while(at<sizeof(actual) && ((uint8_t *)expected)[at]==((uint8_t *)&actual)[at])++at;fprintf(stderr,"combat mismatch roster%u mode%u seat%u frame%u byte%u original%02x paired%02x charge%u/%u command%02x/%02x\n",roster,mode,seat,f,at,((uint8_t *)expected)[at],((uint8_t *)&actual)[at],expected->charge[1],actual.charge[1],expected->command,actual.command);fprintf(stderr,"native control fields AC/59/6E/3B/33/7D/AA/8E/36/37/38/39 original");for(unsigned k=0;k<12;++k)fprintf(stderr," %02x",expected->controls[k]);fprintf(stderr," paired");for(unsigned k=0;k<12;++k)fprintf(stderr," %02x",actual.controls[k]);fprintf(stderr,"\n");if(combat_audio_enabled){fprintf(stderr,"native AUDIO SETUP f%u seat%u camera original%u/%u paired%u/%u bounds original%u/%u/%u/%u paired%u/%u/%u/%u shot0 original",f,seat,combat_reference_camera[seat][f][0],combat_reference_camera[seat][f][1],view_read(0x1e5d),view_read(0x1e60),combat_reference_camera[seat][f][2],combat_reference_camera[seat][f][3],combat_reference_camera[seat][f][4],combat_reference_camera[seat][f][5],view_read(0x1e66),view_read(0x1e68),view_read(0x1e6a),view_read(0x1e6c));for(unsigned k=0;k<12;++k)fprintf(stderr," %02x",expected->shots[k]);fprintf(stderr," paired");for(unsigned k=0;k<12;++k)fprintf(stderr," %02x",actual.shots[k]);fprintf(stderr,"\n");}check(false,"concurrent native command, charge, palette and all owned pools equal the original actor on every frame");}}}
      check((uint8_t)(g_ram[0x9cc]-tick)==(uint8_t)frames,"concurrent combat advances one native world tick per frame");
      if(combat_audio_enabled){unsigned cues[2]={0,0};for(unsigned seat=0;seat<2;++seat)for(unsigned f=0;f<frames;++f)cues[seat]+=combat_reference_cues[seat][f].count;fprintf(stderr,"native AUDIO CADENCE roster%u mode%u frames%u cues%u/%u SPCoriginal%u/%u paired%u\n",roster,mode,frames,cues[0],cues[1],native_spc[0],native_spc[1],g_snes->apu->cycles-spc_start);check(cues[0] && cues[1],"both native actors actually produce their original timed sound requests");uint32_t paired_spc=g_snes->apu->cycles-spc_start,low=native_spc[0]<native_spc[1]?native_spc[0]:native_spc[1],high=native_spc[0]>native_spc[1]?native_spc[0]:native_spc[1];check(paired_spc+32>=low && paired_spc<=high+32,"shared native audio advances one original SPC timeline within one output-sample clock quantum");}
      if(combat_visual_enabled)fprintf(stderr,"native BODY VISUAL roster%u mode%u frames%u original CHR/CGRAM match for both moving actors\n",roster,mode,frames);
      if(!mode)check(peaks[0]>=2 && peaks[1]>=2,"rapid simultaneous firing actually pressures both original native shot limits");else if(mode==1)check(maxcharge[0]>=4 && maxcharge[1]>=4,"staggered releases actually exercise both original native charge progressions");
      fprintf(stderr,"native concurrent combat roster%u mode%u frames%u originalShotPeaks%u/%u originalChargeMax%u/%u\n",roster,mode,frames,peaks[0],peaks[1],maxcharge[0],maxcharge[1]);
      if(mode==4){fprintf(stderr,"native saber damage roster%u activeFrames%u first%u last%u\n",roster,saber_hits,saber_first,saber_last);check(saber_hits>0,"actual native saber swing includes successful damage windows matching the original beside X attacks");}
      if(mode==4)fprintf(stderr,"native full chain roster%u actionMasks%016llx/%016llx shotKindMasks%016llx/%016llx\n",roster,(unsigned long long)actions[0],(unsigned long long)actions[1],(unsigned long long)kinds[0],(unsigned long long)kinds[1]);
      if(earned_saber){unsigned x=setup.players[0].character==X3_COOP_X?0:1;check((actions[x]&(UINT64_C(1)<<(0x5c/2))) && seat_saber_hits[x],"the actual native award enables X's original saber action and damage windows beside living Zero");}
      size_t result=RtlSaveSnapshotToMemory(future,capacity);check(RtlLoadSnapshotFromMemory(base,n),"native concurrent combat restores for complete replay");for(unsigned f=0;f<frames;++f)combat_frame(combat_buttons(mode,0,f)|(combat_buttons(mode,1,f)<<12));same(future,result,work,RtlSaveSnapshotToMemory(work,capacity),"concurrent native firing, charge, owned effects and the complete machine replay exactly");
    }
    if(getenv("X3_COOP_COMBAT_RETIREMENT_ONLY"))combat_retirement(base,n,work,future,capacity);
    if(getenv("X3_COOP_COMBAT_POOL_ONLY"))combat_effect_pressure(base,work,future,capacity);
  }
  combat_visual_finish();
  free(probe);free(trace);
}
