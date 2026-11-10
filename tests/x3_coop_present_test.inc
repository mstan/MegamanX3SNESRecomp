#include "../src/x3_coop_present.h"
static void native_presentation_checks(uint8_t *original,size_t size,uint8_t *work,uint8_t *expected,size_t capacity){
  const X3CoopPresentedFrame *visible=X3CoopPresentationVisible();
  check(visible->players.initialized && visible->count,
        "real native descriptor/Cx4/OAM pipeline supplies visible actor metadata");
  bool native_body=false;
  for(unsigned i=0;i<visible->count;++i)
    if(visible->actors[i].address==X3_COOP_BODY && visible->actors[i].owner==visible->players.anchor+1)native_body=true;
  check(native_body,"visible native draw list retains the primary body's ownership");
  X3CoopPresentedFrame before=*visible;X3CoopState setup=*X3CoopRuntimeState();
  unsigned x=setup.players[1].body[5]|setup.players[1].body[6]<<8;x+=100;
  setup.players[1].body[5]=(uint8_t)x;setup.players[1].body[6]=(uint8_t)(x>>8);
  X3CoopPresentationBegin(&setup,g_ram,before.camera[0]+16,before.camera[1]);
  X3CoopPresentationSubmit(&setup,g_ram,X3_COOP_BODY,setup.current+1);
  check(!memcmp(&before,X3CoopPresentationVisible(),sizeof(before)),
        "queued actor motion cannot advance displayed metadata");
  X3CoopPresentationComplete();
  check(!memcmp(&before,X3CoopPresentationVisible(),sizeof(before)),
        "completed next list retains the previous displayed actors until OAM upload");
  X3CoopPresentationUpload();visible=X3CoopPresentationVisible();
  check((visible->players.players[1].body[5]|visible->players.players[1].body[6]<<8)==x &&
        visible->camera[0]==before.camera[0]+16 && visible->count==1,
        "OAM upload advances the matching private body, camera and native draw list together");
  size_t prepared=RtlSaveSnapshotToMemory(work,capacity),after_size;
  check(prepared!=0,"queued/completed/visible presentation pipeline saves");
  check(RtlLoadSnapshotFromMemory(work,prepared),"presentation pipeline restores");
  after_size=RtlSaveSnapshotToMemory(expected,capacity);
  same(work,prepared,expected,after_size,"presentation pipeline immediate restore");
  size_t marker=0;
  for(size_t i=0;i+4<prepared;++i)if(!memcmp(work+i,"X3P1",4)){marker=i;break;}
  check(marker!=0,"presentation codec chunk is present");
  work[marker+6]=2;
  check(!RtlLoadSnapshotFromMemory(work,prepared),"malformed visible/queued actor identity fails preflight");
  size_t unchanged=RtlSaveSnapshotToMemory(work,capacity);
  same(expected,after_size,work,unchanged,"refused presentation metadata leaves the entire machine unchanged");
  check(RtlLoadSnapshotFromMemory(original,size),"presentation probes restore their starting machine");
}
