#include "x3_coop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t ram[0x20000],before[0x20000];
int main(void) {
  X3CoopState s;
  memset(ram,0x5a,sizeof(ram));
  CHECK(X3CoopInit(&s,ram,X3_COOP_X));
  CHECK(X3CoopValid(&s));
  s.players[1].body[0x27]=28;
  memcpy(before,ram,sizeof(ram));
  CHECK(X3CoopProject(&s,ram,1));
  CHECK(ram[X3_COOP_BODY+0x27]==28);
  CHECK(!memcmp(ram+0xcc8,before+0xcc8,0x10d8-0xcc8));
  CHECK(!memcmp(ram+0x1318,before+0x1318,0x1f1e - 0x1318));
  CHECK(!memcmp(ram+0x1f1f,before+0x1f1f,sizeof(ram)-0x1f1f));
  ram[X3_COOP_BODY+5]=42;
  CHECK(X3CoopProject(&s,ram,0));
  CHECK(s.players[1].body[5]==42);
  CHECK(ram[X3_COOP_BODY+5]==0x5a);
  memset(ram+X3_COOP_PALETTE,0x22,X3_COOP_PALETTE_BYTES);
  CHECK(X3CoopProject(&s,ram,1));
  CHECK(s.players[0].palette[0]==0x22 && ram[X3_COOP_PALETTE]==0x5a);
  CHECK(ram[0xa1]==1);
  CHECK(X3CoopProject(&s,ram,0));
  CHECK(ram[X3_COOP_PALETTE]==0x22);
  ram[X3_COOP_BODY+5]=10;ram[X3_COOP_BODY+6]=0;
  ram[X3_COOP_BODY+8]=100;ram[X3_COOP_BODY+9]=0;
  ram[X3_COOP_BODY+0x27]=16;
  s.players[1].body[5]=90;s.players[1].body[6]=0;
  s.players[1].body[8]=100;s.players[1].body[9]=0;
  s.players[1].body[0x27]=16;
  CHECK(X3CoopNearest(&s,ram,80,100)==1);
  s.anchor=1;CHECK(X3CoopNearest(&s,ram,50,100)==1);
  s.players[1].body[0x27]=0;CHECK(X3CoopNearest(&s,ram,90,100)==0);
  s.anchor=0;
  for(unsigned first=0;first<2;++first) {
    CHECK(X3CoopInit(&s,ram,first));
    CHECK(X3CoopFall(&s,first));
    CHECK(s.anchor==(first^1) && !s.wipe);
    CHECK(!X3CoopRejoin(&s,first));
    CHECK(!X3CoopTeamRetry(&s));
    CHECK(!X3CoopWithdraw(&s,first^1));
    CHECK(!X3CoopProject(&s,ram,first));
    CHECK(X3CoopFall(&s,first^1));
    CHECK(s.wipe);
    CHECK(!X3CoopFall(&s,first));
    CHECK(X3CoopTeamRetry(&s));
    CHECK(!s.wipe && s.players[0].life==X3_COOP_ALIVE && s.players[1].life==X3_COOP_ALIVE);
  }
  CHECK(X3CoopWithdraw(&s,0));
  CHECK(!X3CoopWithdraw(&s,1));
  CHECK(X3CoopRejoin(&s,0));
  CHECK(X3CoopWithdraw(&s,0));
  CHECK(X3CoopFall(&s,1));
  CHECK(s.wipe && !X3CoopRejoin(&s,0));
  uint8_t data[4096];size_t n=X3CoopEncodedSize();
  CHECK(n<=sizeof(data) && X3CoopEncode(&s,data,n));
  X3CoopState copy={0};CHECK(X3CoopDecode(&copy,data,n));
  CHECK(!memcmp(&s,&copy,sizeof(s)));
  data[6]=2;X3CoopState original=copy;
  CHECK(!X3CoopDecode(&copy,data,n));
  CHECK(!memcmp(&original,&copy,sizeof(copy)));
  CHECK(!X3CoopDecode(&copy,data,n-1));
  X3CoopState impossible={0};impossible.players[0].life=X3_COOP_ALIVE;
  CHECK(!X3CoopValid(&impossible));
  impossible=s;impossible.wipe=0;
  CHECK(!X3CoopValid(&impossible));
  /* Physical B mapped to jump by the native mask table. Disabled and compound
   * mappings must behave exactly as the cartridge's CMP/ROR implementation. */
  uint8_t masks[6]={0x80,0,0,0,0,0};
  CHECK(X3CoopActions(1,masks)==0x4000);
  masks[0]=0xc0;CHECK(X3CoopActions(1,masks)==0);
  CHECK(X3CoopActions(3,masks)==0x4000);
  memset(masks,0,sizeof(masks));CHECK(X3CoopActions(0xfff,masks)==0x0f00);
  masks[3]=0x80;CHECK(X3CoopActions(1,masks)==0x0020);
  masks[3]=0;masks[4]=0x80;CHECK(X3CoopActions(1,masks)==0x0010);
  puts("X3 actor projection, fallen persistence, team retry, input and decode contracts passed");
  return 0;
}
