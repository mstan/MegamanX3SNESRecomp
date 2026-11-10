#include "x3_coop.h"
#include <string.h>

/* Native $00:E49E..E551. Engine input uses B,Y,Select,Start,Up,Down,
 * Left,Right,A,X,L,R; the cartridge remaps the six action buttons. */
uint16_t X3CoopActions(uint16_t input,const uint8_t masks[6]) {
  if(!masks)return 0;
  unsigned native=0;
  for(unsigned bit=0;bit<12;++bit)if(input&(1u<<bit))native|=0x8000u>>bit;
  unsigned buttons=((native&255)>>2&0x3c)|(native>>8&0xc0)|(native>>12&3);
  static const uint16_t bits[6]={0x4000,0x8000,0x0080,0x0020,0x0010,0x1000};
  unsigned actions=native&0x0f00;
  for(unsigned i=0;i<6;++i)if(masks[i] && (buttons&masks[i])==masks[i])actions|=bits[i];
  return (uint16_t)actions;
}

static void capture(X3CoopPlayer *p, const uint8_t *ram) {
  memcpy(p->body,ram+X3_COOP_BODY,sizeof(p->body));
  memcpy(p->auxiliaries,ram+X3_COOP_AUX,sizeof(p->auxiliaries));
  memcpy(p->shots,ram+X3_COOP_SHOTS,sizeof(p->shots));
  memcpy(p->palette,ram+X3_COOP_PALETTE,sizeof(p->palette));
  p->shot_command=ram[0x1f1e];
  p->maximum_health=ram[0x1fd2];
}
bool X3CoopValid(const X3CoopState *s) {
  if(!s || s->initialized>1 || s->current>1 || s->anchor>1 || s->wipe>1)return false;
  for(unsigned i=0;i<2;++i)
    if(s->players[i].life>X3_COOP_FALLEN || s->players[i].character>1 ||
       (s->initialized && (s->players[i].maximum_health<1 || s->players[i].maximum_health>32)))return false;
  if(!s->initialized)return !s->wipe && !s->current && !s->anchor &&
      s->players[0].life==X3_COOP_ABSENT && s->players[1].life==X3_COOP_ABSENT;
  if(s->players[0].character==s->players[1].character)return false;
  bool alive=s->players[0].life==X3_COOP_ALIVE || s->players[1].life==X3_COOP_ALIVE;
  if(!alive && !s->wipe)return false;
  if(s->wipe && alive)return false;
  return !alive || s->players[s->anchor].life==X3_COOP_ALIVE;
}
bool X3CoopInit(X3CoopState *s,const uint8_t ram[0x20000],unsigned p1) {
  if(!s || !ram || p1>1)return false;
  memset(s,0,sizeof(*s));
  s->initialized=1;
  /* Enrollment receives the native X body, regardless of controller seat. */
  s->current=s->anchor=(uint8_t)p1;
  for(unsigned i=0;i<2;++i) {
    s->players[i].character=(uint8_t)(p1^i);
    s->players[i].life=X3_COOP_ALIVE;
    capture(&s->players[i],ram);
    if(i==p1)s->players[i].previous_actions=(uint16_t)(ram[X3_COOP_BODY+0x38]|ram[X3_COOP_BODY+0x39]<<8);
  }
  /* Fresh partner: no inherited projectiles, charge effects or armor tasks. */
  X3CoopPlayer *zero=&s->players[p1^1];
  memset(zero->auxiliaries,0,sizeof(zero->auxiliaries));
  memset(zero->shots,0,sizeof(zero->shots));
  zero->shot_command=0;
  zero->maximum_health=28;zero->body[0x27]=28;
  return true;
}
void X3CoopCapture(X3CoopState *s,const uint8_t ram[0x20000]) {
  if(s && s->initialized && s->current<2 && s->players[s->current].life==X3_COOP_ALIVE)
    capture(&s->players[s->current],ram);
}
unsigned X3CoopNearest(const X3CoopState *s,const uint8_t ram[0x20000],unsigned x,unsigned y) {
  unsigned chosen=s->anchor;uint64_t best=UINT64_MAX;
  for(unsigned i=0;i<2;++i) {
    unsigned seat=s->anchor^i;
    const uint8_t *b=seat==s->current?ram+X3_COOP_BODY:s->players[seat].body;
    if(s->players[seat].life!=X3_COOP_ALIVE || !(b[0x27]&127) || b[2]==0x0c)continue;
    int64_t dx=(int)(b[5]|b[6]<<8)-(int)x,dy=(int)(b[8]|b[9]<<8)-(int)y;
    uint64_t distance=(uint64_t)(dx*dx+dy*dy);
    if(distance<best){best=distance;chosen=seat;}
  }
  return chosen;
}
static bool project(X3CoopState *s,uint8_t ram[0x20000],unsigned seat,bool retired) {
  if(!X3CoopValid(s) || !s->initialized || seat>1 ||
     (retired?s->players[seat].life==X3_COOP_ALIVE:s->players[seat].life!=X3_COOP_ALIVE))return false;
  X3CoopCapture(s,ram);
  const X3CoopPlayer *p=&s->players[seat];
  memcpy(ram+X3_COOP_BODY,p->body,sizeof(p->body));
  memcpy(ram+X3_COOP_AUX,p->auxiliaries,sizeof(p->auxiliaries));
  memcpy(ram+X3_COOP_SHOTS,p->shots,sizeof(p->shots));
  if(memcmp(ram+X3_COOP_PALETTE,p->palette,sizeof(p->palette))) {
    memcpy(ram+X3_COOP_PALETTE,p->palette,sizeof(p->palette));
    ram[0xa1]=1; /* Request the native next-NMI palette upload. */
  }
  ram[0x1f1e]=p->shot_command;
  ram[0x1fd2]=p->maximum_health;
  if(retired)ram[X3_COOP_BODY+0x27]=0;
  s->current=(uint8_t)seat;
  return true;
}
bool X3CoopProject(X3CoopState *s,uint8_t ram[0x20000],unsigned seat){return project(s,ram,seat,false);}
bool X3CoopProjectRetired(X3CoopState *s,uint8_t ram[0x20000],unsigned seat){return project(s,ram,seat,true);}
bool X3CoopFall(X3CoopState *s,unsigned seat) {
  if(!X3CoopValid(s) || !s->initialized || seat>1 || s->players[seat].life!=X3_COOP_ALIVE)return false;
  s->players[seat].life=X3_COOP_FALLEN;
  memset(s->players[seat].shots,0,sizeof(s->players[seat].shots));
  memset(s->players[seat].auxiliaries,0,sizeof(s->players[seat].auxiliaries));
  s->players[seat].shot_command=0;
  unsigned partner=seat^1;
  if(s->players[partner].life==X3_COOP_ALIVE)s->anchor=(uint8_t)partner;
  else s->wipe=1;
  return true;
}
bool X3CoopWithdraw(X3CoopState *s,unsigned seat) {
  if(!X3CoopValid(s) || !s->initialized || seat>1 || s->players[seat].life!=X3_COOP_ALIVE || s->players[seat^1].life!=X3_COOP_ALIVE)return false;
  s->players[seat].life=X3_COOP_ABSENT;
  s->anchor=(uint8_t)(seat^1);
  return true;
}
bool X3CoopRejoin(X3CoopState *s,unsigned seat) {
  if(!X3CoopValid(s) || !s->initialized || seat>1 || s->wipe || s->players[seat].life!=X3_COOP_ABSENT)return false;
  s->players[seat].life=X3_COOP_ALIVE;
  return true;
}
bool X3CoopTeamRetry(X3CoopState *s) {
  if(!X3CoopValid(s) || !s->initialized || !s->wipe)return false;
  /* The runtime must initialize the new native bodies before enrollment. */
  for(unsigned i=0;i<2;++i) {
    memset(s->players[i].body,0,sizeof(s->players[i].body));
    memset(s->players[i].auxiliaries,0,sizeof(s->players[i].auxiliaries));
    memset(s->players[i].shots,0,sizeof(s->players[i].shots));
    s->players[i].shot_command=0;
    s->players[i].previous_actions=0;
    s->players[i].life=X3_COOP_ALIVE;
  }
  s->current=s->anchor=s->wipe=0;
  return true;
}
/* Pointer-free wire representation, independent of host struct padding. */
enum { PLAYER_BYTES=X3_COOP_BODY_BYTES+X3_COOP_AUX_BYTES+X3_COOP_SHOT_BYTES+X3_COOP_PALETTE_BYTES+6,
       STATE_BYTES=4+4+2*PLAYER_BYTES };
size_t X3CoopEncodedSize(void) {return STATE_BYTES;}
bool X3CoopEncode(const X3CoopState *s,void *out,size_t size) {
  if(!out || size!=STATE_BYTES || !X3CoopValid(s))return false;
  uint8_t *p=out;memcpy(p,"X3C4",4);p+=4;
  *p++=s->initialized;*p++=s->current;*p++=s->anchor;*p++=s->wipe;
  for(unsigned i=0;i<2;++i) {
    const X3CoopPlayer *a=&s->players[i];
    memcpy(p,a->body,sizeof(a->body));p+=sizeof(a->body);
    memcpy(p,a->auxiliaries,sizeof(a->auxiliaries));p+=sizeof(a->auxiliaries);
    memcpy(p,a->shots,sizeof(a->shots));p+=sizeof(a->shots);
    memcpy(p,a->palette,sizeof(a->palette));p+=sizeof(a->palette);
    *p++=(uint8_t)a->previous_actions;*p++=(uint8_t)(a->previous_actions>>8);
    *p++=a->life;*p++=a->character;*p++=a->shot_command;*p++=a->maximum_health;
  }
  return true;
}
bool X3CoopDecode(X3CoopState *s,const void *data,size_t size) {
  if(!s || !data || size!=STATE_BYTES || memcmp(data,"X3C4",4))return false;
  const uint8_t *p=(const uint8_t *)data+4;X3CoopState v={0};
  v.initialized=*p++;v.current=*p++;v.anchor=*p++;v.wipe=*p++;
  for(unsigned i=0;i<2;++i) {
    X3CoopPlayer *a=&v.players[i];
    memcpy(a->body,p,sizeof(a->body));p+=sizeof(a->body);
    memcpy(a->auxiliaries,p,sizeof(a->auxiliaries));p+=sizeof(a->auxiliaries);
    memcpy(a->shots,p,sizeof(a->shots));p+=sizeof(a->shots);
    memcpy(a->palette,p,sizeof(a->palette));p+=sizeof(a->palette);
    a->previous_actions=(uint16_t)(p[0]|p[1]<<8);p+=2;
    a->life=*p++;a->character=*p++;a->shot_command=*p++;a->maximum_health=*p++;
  }
  if(!X3CoopValid(&v))return false;
  *s=v;return true;
}
