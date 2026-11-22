/* Metadata follows the retail descriptor build -> Cx4 completion -> OAM DMA
 * pipeline. A lag NMI can retain the previous OAM and its private actors. */
#include "x3_coop_present.h"
#include "x3_coop_view.h"
#include "x3_coop_runtime.h"
#include <string.h>

static X3CoopPresentedFrame queued,completed,visible;
static uint8_t body_chr[2][X3_COOP_BODY_CHR_BYTES];
static unsigned word(const uint8_t *p){return p[0]|(unsigned)p[1]<<8;}
static void putword(uint8_t *p,unsigned value){p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);}
void X3CoopPresentationReset(void){memset(&queued,0,sizeof(queued));memset(&completed,0,sizeof(completed));memset(&visible,0,sizeof(visible));memset(body_chr,0,sizeof(body_chr));}
static const uint8_t *source(const uint8_t *rom,size_t size,unsigned address,size_t count){
  size_t offset=((address>>16)&127)*32768+(address&32767);
  return (address&65535)>=32768 && offset<=size && count<=size-offset?rom+offset:NULL;
}
bool X3CoopPresentationBodyDma(unsigned seat,const uint8_t *body,const uint8_t *rom,size_t size){
  if(seat>1 || !body || !rom)return false;
  unsigned base=0x850000|word(body+0x31),pose=body[0x17]&127;
  const uint8_t *p=source(rom,size,base+pose*2,2);if(!p)return false;
  unsigned list=(base&0xff0000)|((base+word(p))&65535);
  for(unsigned i=0;i<32;++i,list+=6){
    p=source(rom,size,list,6);if(!p)return false;
    if(!p[0])return true;
    unsigned target=word(p+4),dest=((target&32767)-0x6000)*2,count=p[0]*16;
    const uint8_t *bytes=source(rom,size,word(p+1)|p[3]<<16,count);
    if(!bytes || dest>X3_COOP_BODY_CHR_BYTES || count>X3_COOP_BODY_CHR_BYTES-dest)return false;
    memcpy(body_chr[seat]+dest,bytes,count);
    if(target&32768)return true;
  }
  return false;
}
void X3CoopPresentationEnroll(const X3CoopState *players,const uint16_t *vram,const uint8_t *rom,size_t size){
  memset(body_chr,0,sizeof(body_chr));
  for(unsigned seat=0;seat<2;++seat){
    if(players->players[seat].character==X3_COOP_X)
      for(unsigned i=0;i<X3_COOP_BODY_CHR_BYTES;++i)body_chr[seat][i]=(uint8_t)(vram[0x6000+i/2]>>((i&1)*8));
    else X3CoopPresentationBodyDma(seat,players->players[seat].body,rom,size);
  }
}
void X3CoopPresentationBegin(const X3CoopState *p,const uint8_t *ram,unsigned x,unsigned y){
  memset(&queued,0,sizeof(queued));queued.players=*p;X3CoopCapture(&queued.players,ram);
  memcpy(queued.weapon_energy,ram+0x1fbb,sizeof(queued.weapon_energy));
  memcpy(queued.hud_palette,ram+0x440,sizeof(queued.hud_palette));
  X3CoopRuntimeScenePresentation(&queued.players);
  memcpy(queued.body_chr,body_chr,sizeof(body_chr));
  queued.camera[0]=(uint16_t)x;queued.camera[1]=(uint16_t)y;
  bool scene=X3CoopRuntimeViewScene();
  for(unsigned seat=0;seat<2;++seat){
    X3CoopView v=X3CoopViewsForPlayer(ram,&queued.players,seat,scene);
    queued.views[seat][0]=(uint16_t)v.x;queued.views[seat][1]=(uint16_t)v.y;
  }
}
void X3CoopPresentationSubmit(const X3CoopState *p,const uint8_t *ram,unsigned address,unsigned owner){
  if(!queued.players.initialized || queued.count==X3_COOP_WORLD_DRAWS || address>0x1fe0 || owner>2)return;
  X3CoopWorldDraw *draw=&queued.actors[queued.count++];
  draw->address=(uint16_t)address;draw->owner=(uint8_t)owner;memcpy(draw->body,ram+address,32);
  /* The native player submission, rather than a later physics poll, owns
   * the displayed primary pose. The private partner is frozen at list start. */
  if(address==X3_COOP_BODY && owner && owner-1==p->current){
    X3CoopState copy=*p;X3CoopCapture(&copy,ram);
    queued.players.players[owner-1]=copy.players[owner-1];
    X3CoopRuntimeDoorLeadPresentation(&queued.players,owner-1);
    memcpy(queued.body_chr[owner-1],body_chr[owner-1],X3_COOP_BODY_CHR_BYTES);
  }
}
void X3CoopPresentationComplete(void){completed=queued;}
void X3CoopPresentationUpload(void){visible=completed;}
const X3CoopPresentedFrame *X3CoopPresentationVisible(void){return &visible;}
const uint8_t *X3CoopPresentationObjectTile(const X3CoopWorldDraw *draw,unsigned address,const uint8_t *rom,size_t size){
  if(!draw || !draw->owner || draw->owner>2 || draw->body[10]!=0x34 || draw->body[0x16]!=0x9e || (address&15))return NULL;
  unsigned character=visible.players.players[draw->owner-1].character,base=0,target=0;
  /* Actual native charged-shot DMA payloads. X and Zero's second shot share
   * 6460..647F even while an earlier creator's particles remain alive. */
  if(address>=0x6460 && address<0x6480){target=0x6460;base=character==X3_COOP_X?0x3c8b60:0x32e920;}
  else if((address>=0x6360 && address<0x6380) || (address>=0x63e0 && address<0x6400)){
    /* The first payload can occupy either native slot; the particle's tile
     * offset follows its parent. Original tables05:DCA1 /05:DE0C. */
    target=address>=0x63e0?0x63e0:0x6360;base=character==X3_COOP_X?0x3c8b20:0x32e8a0;
  }
  return base?source(rom,size,base+(address-target)*2,32):NULL;
}
static size_t frame_size(void){return X3CoopEncodedSize()+12+1+X3_COOP_WORLD_DRAWS*35+sizeof(body_chr)+18+64;}
size_t X3CoopPresentationEncodedSize(void){return sizeof(body_chr)+frame_size()*3;}
static void encode_frame(const X3CoopPresentedFrame *frame,uint8_t *bytes){
  X3CoopEncode(&frame->players,bytes,X3CoopEncodedSize());memcpy(bytes,"X3P5",4);bytes+=X3CoopEncodedSize();
  for(unsigned i=0;i<2;++i){putword(bytes,frame->camera[i]);bytes+=2;}
  for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<2;++i){putword(bytes,frame->views[seat][i]);bytes+=2;}
  *bytes++=frame->count;
  for(unsigned i=0;i<X3_COOP_WORLD_DRAWS;++i){
    const X3CoopWorldDraw *draw=&frame->actors[i];putword(bytes,draw->address);bytes+=2;*bytes++=draw->owner;
    memcpy(bytes,draw->body,32);bytes+=32;
  }
  memcpy(bytes,frame->body_chr,sizeof(frame->body_chr));bytes+=sizeof(frame->body_chr);
  memcpy(bytes,frame->weapon_energy,sizeof(frame->weapon_energy));bytes+=sizeof(frame->weapon_energy);
  memcpy(bytes,frame->hud_palette,sizeof(frame->hud_palette));
}
static bool decode_frame(X3CoopPresentedFrame *frame,const uint8_t *bytes){
  memset(frame,0,sizeof(*frame));
  uint8_t context[4096];
  if(memcmp(bytes,"X3P5",4))return false;
  memcpy(context,bytes,X3CoopEncodedSize());memcpy(context,"X3C5",4);
  if(!X3CoopDecode(&frame->players,context,X3CoopEncodedSize()))return false;
  bytes+=X3CoopEncodedSize();
  for(unsigned i=0;i<2;++i){frame->camera[i]=(uint16_t)word(bytes);bytes+=2;if(frame->camera[i]>8192)return false;}
  for(unsigned seat=0;seat<2;++seat)for(unsigned i=0;i<2;++i){
    frame->views[seat][i]=(uint16_t)word(bytes);bytes+=2;if(frame->views[seat][i]>8192)return false;
  }
  frame->count=*bytes++;if(frame->count>X3_COOP_WORLD_DRAWS || (!frame->players.initialized && frame->count))return false;
  for(unsigned i=0;i<X3_COOP_WORLD_DRAWS;++i){
    X3CoopWorldDraw *draw=&frame->actors[i];draw->address=(uint16_t)word(bytes);bytes+=2;draw->owner=*bytes++;
    if(draw->address>0x1fe0 || draw->owner>2)return false;
    memcpy(draw->body,bytes,32);bytes+=32;
  }
  memcpy(frame->body_chr,bytes,sizeof(frame->body_chr));bytes+=sizeof(frame->body_chr);
  memcpy(frame->weapon_energy,bytes,sizeof(frame->weapon_energy));bytes+=sizeof(frame->weapon_energy);
  memcpy(frame->hud_palette,bytes,sizeof(frame->hud_palette));
  return true;
}
void X3CoopPresentationSave(SaveLoadInfo *io){
  uint8_t bytes[32768];const X3CoopPresentedFrame *frames[]={&queued,&completed,&visible};
  io->func(io,body_chr,sizeof(body_chr));
  for(unsigned i=0;i<3;++i){encode_frame(frames[i],bytes);io->func(io,bytes,frame_size());}
}
bool X3CoopPresentationLoad(SaveLoadInfo *io){
  uint8_t bytes[32768],chr[sizeof(body_chr)];X3CoopPresentedFrame frames[3];
  io->func(io,chr,sizeof(chr));
  for(unsigned i=0;i<3;++i){io->func(io,bytes,frame_size());if(!decode_frame(&frames[i],bytes))return false;}
  queued=frames[0];completed=frames[1];visible=frames[2];memcpy(body_chr,chr,sizeof(chr));return true;
}
bool X3CoopPresentationPreflight(const void *data,size_t size){
  if(!data || size!=X3CoopPresentationEncodedSize())return false;
  X3CoopPresentedFrame frame;const uint8_t *bytes=(const uint8_t *)data+sizeof(body_chr);
  for(unsigned i=0;i<3;++i)if(!decode_frame(&frame,bytes+i*frame_size()))return false;
  return true;
}
