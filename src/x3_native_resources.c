/* Read-only equivalents of native00:B730 decompression,00:B2DB upload
 * segmentation and01:804A palette lists. No runtime-ROM patches or uploads. */
#include "x3_native_resources.h"
static bool read_rom(const uint8_t *rom,size_t size,unsigned address,unsigned bytes,unsigned *value){
  size_t at=((address>>16)&127)*32768+(address&32767);
  if(!rom || (address&65535)<32768 || at>size || bytes>size-at)return false;
  *value=0;for(unsigned i=0;i<bytes;++i)*value|=(unsigned)rom[at+i]<<(i*8);return true;
}
static bool next(const uint8_t *rom,size_t size,unsigned *address,unsigned *value){
  if(!read_rom(rom,size,*address,1,value))return false;
  if((++*address&65535)==0)*address+=32768;
  return true;
}
bool X3ResourceDecode(const uint8_t *rom,size_t size,unsigned key,uint8_t *out,size_t capacity,size_t *length){
  unsigned source,total;
  if(key>=X3_RESOURCE_KEYS || !out || !length ||
     !read_rom(rom,size,0x06f732+key*5,3,&source) ||
     !read_rom(rom,size,0x06f735+key*5,2,&total) || !total || total>capacity)return false;
  unsigned written=0;
  while(written<total){
    unsigned flags;if(!next(rom,size,&source,&flags))return false;
    for(unsigned mask=128;mask && written<total;mask>>=1){
      unsigned value;if(!next(rom,size,&source,&value))return false;
      if(flags&mask){
        unsigned low;if(!next(rom,size,&source,&low))return false;
        unsigned count=value>>2,distance=((value&3)<<8)|low;
        if(!count || !distance || distance>written || count>total-written)return false;
        while(count--){out[written]=out[written-distance];++written;}
      }else out[written++]=(uint8_t)value;
    }
  }
  *length=total;return true;
}
bool X3ResourcePalette(const uint8_t *rom,size_t size,unsigned key,unsigned offset,uint16_t colors[256]){
  unsigned list;
  if(!colors || key>510 || (key&1) || offset>255 || !read_rom(rom,size,0x068180+key,2,&list))return false;
  for(unsigned i=0;i<64;++i,list+=4){
    unsigned count,source,dest;
    if(!read_rom(rom,size,0x060000|list,1,&count))return false;
    if(!count)return true;
    if(!read_rom(rom,size,0x060001+list,2,&source) ||
       !read_rom(rom,size,0x060003+list,1,&dest) || dest+offset+count>256)return false;
    for(unsigned j=0;j<count;++j){unsigned color;
      if(!read_rom(rom,size,0x0c0000|((source+j*2)&65535),2,&color))return false;
      colors[dest+offset+j]=(uint16_t)(color&32767);
    }
  }
  return false;
}
bool X3ResourceLayout(const uint8_t *rom,size_t size,unsigned key,unsigned target,X3ResourceSegment segments[X3_RESOURCE_SEGMENTS],unsigned *count){
  unsigned list,used=0,source=0;
  if(key>=X3_RESOURCE_KEYS || !segments || !count || target>=32768 ||
     !read_rom(rom,size,0x08d7c9+key*2,2,&list))return false;
  list+=0x08d7c9;
  for(unsigned i=0;i<128;++i){
    unsigned length,delta;
    if(!read_rom(rom,size,list++,1,&length))return false;
    if(!length){*count=used;return true;}
    if(length==255)continue; /* Original queue/yield marker, no file bytes. */
    if(used==X3_RESOURCE_SEGMENTS || !read_rom(rom,size,list++,1,&delta))return false;
    unsigned destination=(target&255)|((((target>>8)+(delta&127))&255)<<8);
    length*=16;
    if(destination>=32768 || destination*2+length>65536 || source+length>X3_RESOURCE_FILE_MAX)return false;
    segments[used++]=(X3ResourceSegment){(uint16_t)destination,(uint16_t)source,(uint16_t)length};source+=length;
    if(delta&128){*count=used;return true;}
  }
  return false;
}
