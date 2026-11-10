#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
enum {X3_RESOURCE_KEYS=224,X3_RESOURCE_FILE_MAX=32768,X3_RESOURCE_SEGMENTS=64};
typedef struct X3ResourceSegment {uint16_t destination,source,length;} X3ResourceSegment;
bool X3ResourceDecode(const uint8_t *rom,size_t size,unsigned key,uint8_t *out,size_t capacity,size_t *length);
bool X3ResourcePalette(const uint8_t *rom,size_t size,unsigned key,unsigned offset,uint16_t colors[256]);
bool X3ResourceLayout(const uint8_t *rom,size_t size,unsigned key,unsigned target,X3ResourceSegment segments[X3_RESOURCE_SEGMENTS],unsigned *count);
