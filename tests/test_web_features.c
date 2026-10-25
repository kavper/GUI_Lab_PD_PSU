#include "psu_web_features.h"
#include "psu_app.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static char response[8192];
static void job(const char *body) {
    assert(psu_web_request("POST","/api/feature",body,"192.168.0.20",response,sizeof(response))>0);
    assert(strstr(response,"queued"));
    psu_web_features_poll();
    assert(psu_web_features_state("/api/features",response,sizeof(response))>0);
}
int main(int argc,char **argv) {
    psu_app_init();
    const char *invalid[]={"{}","{\"op\":1}","{\"op\":41,\"extra\":1}","{\"op\":41,\"op\":41}","{\"op\":-1}","{\"op\":041}","{\"op\":4294967296}","{\"op\":41}x"};
    for(unsigned n=0;n<sizeof(invalid)/sizeof(invalid[0]);n++) {
        psu_web_features_submit(invalid[n],response,sizeof(response));assert(strstr(response,"invalid fields"));
    }
    job("{\"op\":1,\"index\":0,\"v\":12345,\"i\":678}");
    assert(strstr(response,"\"ok\":1"));assert(psu_preset_get(0)->voltage_mv==12345);
    job("{\"op\":1,\"index\":0,\"v\":27001,\"i\":678}");
    assert(strstr(response,"\"ok\":0"));assert(psu_preset_get(0)->voltage_mv==12345);
    job("{\"op\":40,\"dark\":1,\"accent\":2,\"volume\":23,\"sound\":1,\"tone\":2,\"feedback\":3}");
    PsuAppearance a;psu_appearance_get(&a);assert(a.accent==2&&a.sound_volume==23);assert(psu_web_features_gui_events()==1);assert(!psu_web_features_gui_events());
    job("{\"op\":40,\"dark\":1,\"accent\":4,\"volume\":23,\"sound\":1,\"tone\":2,\"feedback\":3}");
    assert(strstr(response,"\"ok\":0"));psu_appearance_get(&a);assert(a.accent==2);
    job("{\"op\":20,\"chem\":2,\"cells\":4,\"i\":1000,\"term\":100,\"target\":14000}");
    assert(strstr(response,"\"ok\":1"));assert(psu_charger()->profile.chemistry==CHEM_LIFEPO4);assert(!psu_charger()->profile.polarity_checked);
    job("{\"op\":21,\"polarity\":0}");assert(strstr(response,"\"ok\":0"));assert(!psu_charger()->running);
    PsuSequencer *s=psu_sequencer();s->config_locked=1;
    unsigned count=s->count;job("{\"op\":11}");assert(s->count==count&&strstr(response,"\"ok\":0"));s->config_locked=0;
    job("{\"op\":13,\"mode\":1,\"loops\":42,\"stop\":0}");assert(s->loops_requested==42);
    psu_web_features_submit("{\"op\":41}",response,sizeof(response));assert(strstr(response,"queued"));
    psu_remote_enable(0);psu_web_features_poll();assert(!psu_web_features_gui_events());psu_remote_enable(1);
    psu_web_features_init();for(unsigned n=0;n<3;n++){psu_web_features_submit("{\"op\":41}",response,sizeof(response));assert(strstr(response,"queued"));}
    psu_web_features_submit("{\"op\":41}",response,sizeof(response));assert(strstr(response,"queue busy"));
    psu_web_features_init();
    assert(!psu_web_features_state("/api/features",response,32));
    for(unsigned n=0;n<128;n++){psu_charger()->trace_time[n]=n*360000;psu_charger()->trace_mv[n]=27000;psu_charger()->trace_ma[n]=5000;}
    psu_charger()->trace_count=128;
    assert(psu_web_features_state("/api/features",response,sizeof(response))>0);
    if(argc>1){FILE *f=fopen(argv[1],"wb");assert(f);fputs(response,f);fclose(f);}
    assert(psu_web_features_state("/api/diagnostics",response,sizeof(response))>0);
    assert(psu_web_features_state("/api/pd",response,sizeof(response))>0);
    puts("PASS native web features: strict fields, bounded queue, execution-time access, preset limits, charger polarity, chemistry, automation locks, shared appearance and bounded 128-point serialization");
    return 0;
}
