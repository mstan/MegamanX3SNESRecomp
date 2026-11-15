/* Exercise the shipped launcher's provider callbacks without opening a UI. */
static RecompLauncherCModFeature packaged_feature(const RecompLauncherCModProvider *p,const char *package,const char *id){
  RecompLauncherCModFeature f={0};
  for(int i=0;i<p->feature_count(p->ctx);++i)if(p->feature_get(p->ctx,i,&f) && !strcmp(f.package_id,package) && !strcmp(f.id,id))return f;
  check(false,"packaged launcher exposes the requested feature");return f;
}
static RecompLauncherCModOption packaged_option(const RecompLauncherCModProvider *p,const char *feature,const char *id){
  RecompLauncherCModOption o={0};
  for(int i=0;i<32 && p->feature_option_get(p->ctx,"megaman-x3.coop",feature,i,&o);++i)if(!strcmp(o.id,id))return o;
  check(false,"packaged launcher exposes the requested option");return o;
}
static void native_packaged_provider(const char *catalog,const char *rom){
  const char *game="megaman-x3-us",*digest="65b03268afac296330e8ff8d60dd0825879e13ed658b37713c034a3bd074f1d7",*coop="megaman-x3.coop",*ws="megaman-x3.enhancement.widescreen";
  check(snes_mod_runtime_initialize_c(catalog,game,digest),"fresh extracted catalog initializes through the production launcher provider");
  const RecompLauncherCModProvider *p=snes_mod_runtime_launcher_provider_c();check(p && p->feature_enable && p->feature_set_option && p->commit,"the shipped provider supplies the launcher's actual editing callbacks");
  RecompLauncherCModFeature c=packaged_feature(p,coop,"coop"),v=packaged_feature(p,coop,"view"),w=packaged_feature(p,ws,"widescreen");
  check(!c.enabled && v.enabled && w.enabled && !c.hidden && !c.has_error && !v.has_error && !w.has_error && strstr(w.name,"Fit to screen"),"fresh launcher defaults expose optional co-op and enabled adaptive Fit with friendly labels");
  const char *features[]={"coop","coop","coop","coop","coop","view","view"};
  const char *ids[]={"player1","cameras","zero_rules","learner","doors","layout","hud"};
  const char *defaults[]={"x","independent","full","off","either","auto","compact"};
  const char *changed[]={"zero","unified","stock","on","both","shared","horizontal"};
  for(unsigned i=0;i<7;++i){RecompLauncherCModOption o=packaged_option(p,features[i],ids[i]);
    check(!strcmp(o.value,defaults[i]) && o.label[0] && strcmp(o.label,o.id) && !o.disabled && o.choice_count>=2 && o.choice_count<=5,"co-op controls expose their native defaults and readable enabled choices");
    RecompLauncherCModChoice choices[5]={0};for(int j=0;j<o.choice_count;++j){check(p->feature_choice_get(p->ctx,coop,features[i],ids[i],j,&choices[j]) && choices[j].label[0],"every co-op choice has a readable launcher label");for(int k=0;k<j;++k)check(strcmp(choices[j].value,choices[k].value) && strcmp(choices[j].label,choices[k].label),"launcher choices contain no equivalent duplicate labels or values");}
  }
  check(p->feature_enable(p->ctx,coop,"coop",1),"the actual launcher control enables co-op");
  for(unsigned pass=0;pass<2;++pass){const char **want=pass?defaults:changed;
    for(unsigned i=0;i<7;++i)check(p->feature_set_option(p->ctx,coop,features[i],ids[i],want[i]),"actual launcher option controls accept each supported selection");
    check(p->commit(p->ctx,rom),"the actual PLAY callback validates the original cartridge and persists launcher selections");
    check(snes_mod_runtime_initialize_c(catalog,game,digest),"the production provider reloads saved launcher choices");p=snes_mod_runtime_launcher_provider_c();
    c=packaged_feature(p,coop,"coop");check(c.enabled && !c.has_error,"enabled co-op survives launcher reload without diagnostics");
    for(unsigned i=0;i<7;++i){RecompLauncherCModOption o=packaged_option(p,features[i],ids[i]);check(!strcmp(o.value,want[i]),"each roster, participation, view and ownership-related policy choice persists independently");}
  }
  check(p->feature_enable(p->ctx,ws,"widescreen",0) && p->commit(p->ctx,rom),"the launcher's explicit widescreen-off choice commits");
  check(snes_mod_runtime_initialize_c(catalog,game,digest),"saved widescreen choice reloads");p=snes_mod_runtime_launcher_provider_c();w=packaged_feature(p,ws,"widescreen");check(!w.enabled,"the provider retains an explicit saved widescreen-off choice");
  check(p->feature_enable(p->ctx,ws,"widescreen",1) && p->commit(p->ctx,rom),"the launcher can restore adaptive Fit beside enabled co-op");
  puts("PACKAGED X3 LAUNCHER PROVIDER CHECKS PASSED");
}
