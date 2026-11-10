#include "mod_runtime.h"
#include "x3_coop_settings.h"
#include "x3_coop_runtime.h"
#include "x3_coop_view.h"
#include "x3_display.h"
#include <stdlib.h>
#include <string.h>

static const char *package="megaman-x3.coop";
static bool profile_loaded,view_loaded,enabled,independent;
static unsigned player1;

static bool option_is(const char *feature,const char *key,const char *value){
  char option[32]={0};
  return snes_mod_runtime_feature_option_value_c(package,feature,key,option,sizeof(option)) && !strcmp(option,value);
}
static bool selected(void){
  const char *override=getenv("SNESRECOMP_X3_COOP_PROTOTYPE");
  return override?override[0]=='1':snes_mod_runtime_feature_enabled_c(package,"coop")!=0;
}
static bool independent_selected(void){
  const char *override=getenv("SNESRECOMP_X3_COOP_INDEPENDENT");
  return override?override[0]=='1':
      snes_mod_runtime_feature_enabled_c(package,"coop") && option_is("coop","cameras","independent");
}
static void resolve_profile(void){
  if(profile_loaded)return;
  enabled=selected();independent=independent_selected();
  const char *character=getenv("SNESRECOMP_X3_COOP_P1");
  player1=(character?!strcmp(character,"zero"):option_is("coop","player1","zero"))?X3_COOP_ZERO:X3_COOP_X;
  profile_loaded=true;
}
bool X3CoopSettingsEnabled(void){resolve_profile();return enabled;}
void X3CoopSettingsBoot(void){
  resolve_profile();
  X3CoopRuntimeSetP1Character(player1);
  X3CoopViewsConfigure(enabled && independent);
  X3CoopRuntimeEnable(enabled);
}
static void chosen_view(char view[32]){
  strcpy(view,"shared");
  if(snes_mod_runtime_feature_enabled_c(package,"coop") &&
     snes_mod_runtime_feature_enabled_c(package,"view")){
    snes_mod_runtime_feature_option_value_c(package,"view","layout",view,32);
    if(!strcmp(view,"auto"))strcpy(view,independent_selected()?"split":"shared");
  }
  const char *override=getenv("SNESRECOMP_X3_COOP_VIEW");
  if(override && strlen(override)<32)strcpy(view,override);
}
int X3CoopSettingsWindowWidth(int frame_width){
  /* The launcher can ask before plugin activation, then change selections.
   * A window preview must not freeze the simulation's startup selection. */
  char view[32];chosen_view(view);
  return X3Display_GetWindowBaseWidth(selected()?(!strcmp(view,"split")?512:256):frame_width);
}
void X3CoopSettingsPresentation(void){
  if(view_loaded)return;
  char view[32];chosen_view(view);
  /* Only host presentation changes here. Never configure X3CoopViews or
   * write the cached simulation profile from this callback. */
  bool on=selected();
  X3Display_SetLocalSplit(on && !strcmp(view,"split"));
  X3Display_SetPresentationSeat(on && !strcmp(view,"p1")?0:on && !strcmp(view,"p2")?1:-1);
  view_loaded=true;
}
static void reset(void){
  profile_loaded=view_loaded=false;
  enabled=independent=false;player1=X3_COOP_X;
  X3Display_SetLocalSplit(false);X3Display_SetPresentationSeat(-1);
}
static void activate_coop(void){resolve_profile();}
static void activate_view(void){X3CoopSettingsPresentation();}
SNES_MOD_CONSTRUCTOR(x3_register_coop_plugin){
  (void)snes_mod_register_reset_callback(reset);
  (void)snes_mod_register_activation_plugin("megaman-x3.coop",activate_coop);
  (void)snes_mod_register_presentation_plugin("megaman-x3.coop-view",activate_view);
}
