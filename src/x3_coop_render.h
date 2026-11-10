#pragma once
struct PpuExtraObject;
/* Native body art at a presentation-only camera, without selecting an actor. */
const struct PpuExtraObject *X3CoopRenderPlayer(unsigned seat,int camera_x,int camera_y);
void X3CoopRender(void);
void X3CoopRenderRestorePalette(void);
