#pragma once
#include <stdbool.h>

/* The simulation selection is fixed at startup; local view selection never
 * changes the actor roster, camera policy or shared-world activation. */
bool X3CoopSettingsEnabled(void);
void X3CoopSettingsBoot(void);
void X3CoopSettingsResume(void);
void X3CoopSettingsPresentation(void);
int X3CoopSettingsWindowWidth(int frame_width);
