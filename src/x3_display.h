#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Host-only display controls. These never modify emulated SNES state. */
void X3Display_SetWidescreenEnabled(bool enabled);
bool X3Display_IsWidescreenEnabled(void);
bool X3Display_IsWidescreenActive(void);
int X3Display_GetCurrentFrameWidth(void);
void X3Display_SetLocalSplit(bool enabled);
bool X3Display_LocalSplit(void);
void X3Display_SetPresentationSeat(int seat);
int X3Display_PresentationSeat(void);
bool X3Display_RenderedPrivateFrame(void);
struct Ppu;
bool X3Display_BeginIndependentFrame(struct Ppu *ppu);
bool X3Display_BeginLocalSplitFrame(struct Ppu *ppu);
void X3Display_DrawIndependentLine(struct Ppu *ppu,int line);
void X3Display_DrawLocalSplitLine(struct Ppu *ppu,int line);
void X3Display_CaptureLocalSplitNative(const struct Ppu *ppu);
int X3Display_DrawLocalSplit(uint8_t *dst,size_t pitch,const uint8_t *field,int width,int height,double alpha);

typedef struct X3DisplayViewport {
  int x, y, width, height;
} X3DisplayViewport;

/* Pure geometry: SNES active pixels have a 7:6 horizontal pixel aspect. */
int X3Display_ComputeFrameWidth(int drawable_width, int drawable_height,
                                 bool widescreen);
void X3Display_ComputePresentationSize(int frame_width, int frame_height,
                                        int *width, int *height);
void X3Display_ComputeViewport(int source_width, int source_height,
                                int drawable_width, int drawable_height,
                                bool ignore_aspect, bool integer_scale,
                                X3DisplayViewport *viewport);
int X3Display_GetWindowBaseWidth(int frame_width);
int X3Display_GetWindowBaseHeight(void);

/* Reattach a streamed PPU tilemap's 8-bit phase to MMX's full world camera. */
int X3Display_ExpandStageScroll(uint16_t camera, uint16_t ppu_scroll);

#ifdef __cplusplus
}
#endif
