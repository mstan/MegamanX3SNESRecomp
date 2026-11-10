/* Per-player cameras over the native X3 camera and03:DD76 region commands.
 * World commands execute only in the cartridge; this parser reads limits. */
#include "x3_coop_view.h"
#include <string.h>

static X3CoopViewState state;
static const uint8_t *rom;
static size_t rom_size;
static const unsigned targets[4]={0x1e6e,0x1e70,0x1e78,0x1e7e};
static unsigned word(const uint8_t *r){return r[0]|(unsigned)r[1]<<8;}
static void putword(uint8_t *r,unsigned v){r[0]=(uint8_t)v;r[1]=(uint8_t)(v>>8);}
static int clamp(int v,int lo,int hi){if(hi<lo)hi=lo;return v<lo?lo:v>hi?hi:v;}
static unsigned rom_word(unsigned a){size_t p=0x18000+(a&32767);return rom && a>=32768 && p+2<=rom_size?word(rom+p):0;}
static unsigned rom_byte(unsigned a){size_t p=0x18000+(a&32767);return rom && a>=32768 && p<rom_size?rom[p]:0;}
static const uint8_t *body(const uint8_t *r,const X3CoopState *p,unsigned seat){return seat==p->current?r+X3_COOP_BODY:p->players[seat].body;}
static bool living(const uint8_t *r,const X3CoopState *p,unsigned seat){const uint8_t *b=body(r,p,seat);return p->players[seat].life==X3_COOP_ALIVE && (b[0x27]&127) && b[2]!=12;}
void X3CoopViewsConfigure(bool value){if(state.independent!=value){X3CoopViewsReset();state.independent=value;}}
bool X3CoopViewsIndependent(void){return state.independent;}
void X3CoopViewsSetRom(const uint8_t *r,size_t size){rom=r;rom_size=size;}
void X3CoopViewsReset(void){bool mode=state.independent;memset(&state,0,sizeof(state));state.independent=mode;}
const X3CoopViewState *X3CoopViewsState(void){return &state;}
bool X3CoopViewsValidState(const X3CoopViewState *s){
  if(!s || s->valid>3 || s->camera_valid>3 || s->stage>14 || s->initialized>1 || s->independent>1)return false;
  for(unsigned seat=0;seat<2;++seat){
    for(unsigned i=0;i<4;++i)if(s->limits[seat][i]>8192 || s->current[seat][i]>8192)return false;
    for(unsigned i=0;i<2;++i)if(s->camera[seat][i]>8192)return false;
    if((s->valid&(1u<<seat)) && (s->limits[seat][0]>s->limits[seat][1] || s->limits[seat][2]>s->limits[seat][3]))return false;
  }
  return true;
}
void X3CoopViewsSetState(const X3CoopViewState *s){state=*s;}
void X3CoopViewsPlayerLimits(const uint8_t *r,const X3CoopState *p,unsigned seat,int out[4]){
  for(unsigned i=0;i<4;++i)out[i]=word(r+0x1e66+i*2);
  if(state.independent && p->initialized && seat<2 && state.stage==r[0x1fae] && (state.valid&(1u<<seat)))
    for(unsigned i=0;i<4;++i)out[i]=state.limits[seat][i];
}
void X3CoopViewsUpdate(const uint8_t *r,const X3CoopState *p,bool scene,bool menu){
  if(!state.independent || !p->initialized || r[0x1fae]>14)return;
  unsigned stage=r[0x1fae];
  if(!state.initialized || state.stage!=stage){X3CoopViewsReset();state.stage=(uint8_t)stage;state.initialized=1;}
  if(scene || r[0xd1]!=4 || r[0xd2]!=4){state.valid=0;return;}
  if(menu)return;
  /* Native shared encounter locks supersede authored region targets. */
  if(r[0x1f63]){state.valid=0;return;}
  /* Stage14 aliases stage11. Region lists are not in stage order, and
   * stage0's first record is not the lowest record address. Bound both
   * from all15 native stage lists instead of assuming monotonic pointers. */
  unsigned lists[15],data=0xf2cc;
  for(unsigned i=0;i<15;++i){
    lists[i]=rom_word(0xde43+i*2);
    if(lists[i]<0xde61 || lists[i]>=0xe1ef)return;
    unsigned rec=rom_word(lists[i]);
    if(rec<0xe1ef || rec>=0xf2cc)return;
    if(rec<data)data=rec;
  }
  unsigned first=lists[stage],end=data;
  for(unsigned i=0;i<15;++i)if(lists[i]>first && lists[i]<end)end=lists[i];
  if(end<first || ((end-first)&1))return;
  for(unsigned seat=0;seat<2;++seat){
    unsigned bit=1u<<seat;
    if(!living(r,p,seat)){state.valid&=(uint8_t)~bit;continue;}
    if(!(state.valid&bit)){for(unsigned i=0;i<4;++i)state.limits[seat][i]=(uint16_t)word(r+targets[i]);state.valid|=(uint8_t)bit;}
    const uint8_t *b=body(r,p,seat);unsigned x=word(b+5),y=word(b+8);
    for(unsigned e=first;e<end;e+=2){
      unsigned rec=rom_word(e);
      if(rec<data || rec+8>=0xf2cc || x>=rom_word(rec) || x<rom_word(rec+2) || y>=rom_word(rec+4) || y<rom_word(rec+6))continue;
      for(unsigned n=8;n<64 && rec+n<0xf2cc;++n){
        unsigned cmd=rom_byte(rec+n);if(!cmd)break;
        unsigned address=rom_word(0xf2cc + (cmd-1)*4),value=rom_word(0xf2ce + (cmd-1)*4);
        if(value>8192)continue;
        for(unsigned i=0;i<4;++i)if(address==targets[i])state.limits[seat][i]=(uint16_t)value;
      }
    }
  }
}
static int follow(int b,int c,int lo,int hi,int speed){int d=b-c,want=d<lo?b-lo:d>hi?b-hi:c;return clamp(want,c-speed,c+speed);}
X3CoopView X3CoopViewsFollowPlayer(const uint8_t *r,const X3CoopState *p,unsigned seat){
  X3CoopView v=X3CoopViewsForPlayer(r,p,seat,false);
  const uint8_t *b=body(r,p,seat);int speed=word(r+0x1e64);
  v.x=follow(word(b+5),v.x,word(r+0x1e84),word(r+0x1e86),speed);
  v.y=follow(word(b+8),v.y,word(r+0x1e80),word(r+0x1e82),speed);
  return v;
}
static int limit_step(int target,int current,int camera,int b,int previous,int zone,int speed,bool minimum){
  if(target==current)return current;
  if(minimum?target>=camera:target<camera)return minimum?(camera+speed<target?camera+speed:target):(camera-speed>=target?camera-speed:target);
  if((uint16_t)(b-camera-zone+2)<4)return target;
  int movement=(int16_t)(b-previous);
  if(minimum){if(movement>0)movement=0;}else if(movement<0)movement=0;
  int next=camera+movement+(minimum?-speed:speed);
  return minimum?(next>=target?next:target):(next<target?next:target);
}
void X3CoopViewsEndFrame(const uint8_t *r,const X3CoopState *p,bool scene,bool menu){
  if(!state.independent || !p->initialized || scene || r[0xd1]!=4 || r[0xd2]!=4){state.camera_valid=0;return;}
  if(menu)return;
  int zones[4]={word(r+0x1e84),word(r+0x1e86),word(r+0x1e80),word(r+0x1e82)};
  int tracking=word(r+0x1e64),pan=word(r+0x1e62);
  for(unsigned seat=0;seat<2;++seat){
    unsigned bit=1u<<seat;const uint8_t *b=body(r,p,seat);
    if(!living(r,p,seat)){state.camera_valid&=(uint8_t)~bit;continue;}
    if(seat==p->anchor || !(state.valid&bit)){
      state.camera[seat][0]=(uint16_t)word(r+0x1e5d);state.camera[seat][1]=(uint16_t)word(r+0x1e60);
      for(unsigned i=0;i<4;++i)state.current[seat][i]=(uint16_t)word(r+0x1e66+i*2);
      state.camera_valid|=(uint8_t)bit;continue;
    }
    int x=word(b+5),y=word(b+8);
    if(!(state.camera_valid&bit)){
      for(unsigned i=0;i<4;++i)state.current[seat][i]=state.limits[seat][i];
      state.camera[seat][0]=(uint16_t)clamp(x-zones[0],state.current[seat][0],state.current[seat][1]);
      state.camera[seat][1]=(uint16_t)clamp(y-zones[2],state.current[seat][2],state.current[seat][3]);
      state.camera_valid|=(uint8_t)bit;
    }
    int cx=clamp(follow(x,state.camera[seat][0],zones[0],zones[1],tracking),state.current[seat][0],state.current[seat][1]);
    int cy=clamp(follow(y,state.camera[seat][1],zones[2],zones[3],tracking),state.current[seat][2],state.current[seat][3]);
    state.camera[seat][0]=(uint16_t)cx;state.camera[seat][1]=(uint16_t)cy;
    for(unsigned i=0;i<4;++i)state.current[seat][i]=(uint16_t)limit_step(state.limits[seat][i],state.current[seat][i],i<2?cx:cy,i<2?x:y,word(b+(i<2?0x22:0x24)),zones[i],pan,!(i&1));
  }
}
X3CoopView X3CoopViewsForPlayer(const uint8_t *r,const X3CoopState *p,unsigned seat,bool scene){
  X3CoopView v={(int)word(r+0x1e5d),(int)word(r+0x1e60),p->anchor};
  if(!state.independent || !p->initialized || seat>1 || scene || r[0xd1]!=4 || r[0xd2]!=4)return v;
  if(!living(r,p,seat))seat=p->anchor;v.seat=seat;
  if(state.stage==r[0x1fae] && (state.camera_valid&(1u<<seat))){v.x=state.camera[seat][0];v.y=state.camera[seat][1];return v;}
  if(seat==p->anchor)return v;
  int limits[4];X3CoopViewsPlayerLimits(r,p,seat,limits);const uint8_t *b=body(r,p,seat);
  v.x=clamp(word(b+5)-128,limits[0],limits[1]);v.y=clamp(word(b+8)-160,limits[2],limits[3]);return v;
}
void X3CoopViewsAdoptPlayer(uint8_t *r,const X3CoopState *p,unsigned seat,bool scene){
  if(!state.independent || seat>1 || seat==p->anchor || scene || !(state.valid&(1u<<seat)) || !living(r,p,seat))return;
  X3CoopView v=X3CoopViewsForPlayer(r,p,seat,false);
  for(unsigned i=0;i<4;++i){putword(r+0x1e66+i*2,state.current[seat][i]);putword(r+targets[i],state.limits[seat][i]);}
  putword(r+0x1e5d,(unsigned)v.x);putword(r+0x1e7a,(unsigned)v.x);putword(r+0x1e60,(unsigned)v.y);putword(r+0x1e7c,(unsigned)v.y);
}
bool X3CoopViewsContains(const uint8_t *r,const X3CoopState *p,int x,int y,int left,int right,int top,int bottom,int margin,bool scene){
  for(unsigned seat=0;seat<2;++seat){
    if(!living(r,p,seat) || (scene && seat!=p->anchor))continue;
    X3CoopView v=X3CoopViewsForPlayer(r,p,seat,scene);int dx=(int16_t)(x-v.x),dy=(int16_t)(y-v.y);
    if(dx>=-left-margin && dx<right+margin && dy>=-top && dy<bottom)return true;
  }
  return false;
}
bool X3CoopViewsSeen(unsigned flag){return flag>=0xf800 && flag<=0xffff && (state.seen[(flag-0xf800)>>3]&(1u<<(flag&7)));}
void X3CoopViewsMark(unsigned flag,bool seen){if(flag<0xf800 || flag>0xffff)return;unsigned i=(flag-0xf800)>>3,bit=1u<<(flag&7);if(seen)state.seen[i]|=(uint8_t)bit;else state.seen[i]&=(uint8_t)~bit;}
void X3CoopViewsSave(void *out){uint8_t *p=out;*p++=state.independent;*p++=state.valid;*p++=state.camera_valid;*p++=state.stage;*p++=state.initialized;
  for(unsigned seat=0;seat<2;++seat){for(unsigned i=0;i<4;++i){putword(p,state.limits[seat][i]);p+=2;}for(unsigned i=0;i<4;++i){putword(p,state.current[seat][i]);p+=2;}for(unsigned i=0;i<2;++i){putword(p,state.camera[seat][i]);p+=2;}}
  memcpy(p,state.seen,sizeof(state.seen));
}
bool X3CoopViewsLoad(const void *bytes,size_t size,bool apply){
  if(!bytes || size!=X3_COOP_VIEW_STATE_BYTES)return false;const uint8_t *p=bytes;X3CoopViewState next={0};next.independent=*p++;next.valid=*p++;next.camera_valid=*p++;next.stage=*p++;next.initialized=*p++;
  for(unsigned seat=0;seat<2;++seat){for(unsigned i=0;i<4;++i){next.limits[seat][i]=(uint16_t)word(p);p+=2;}for(unsigned i=0;i<4;++i){next.current[seat][i]=(uint16_t)word(p);p+=2;}for(unsigned i=0;i<2;++i){next.camera[seat][i]=(uint16_t)word(p);p+=2;}}
  memcpy(next.seen,p,sizeof(next.seen));if(!X3CoopViewsValidState(&next))return false;if(apply)state=next;return true;
}
