#include <gui/common/SettingsPersistence.hpp>
extern "C" {
#include "psu_app.h"
#include "psu_nvstore.h"
}
#ifdef SIMULATOR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
namespace {
PsuNvStore storage;
uint8_t payload[PSU_NV_PAYLOAD];
uint32_t clockMs;
bool initialized=false, testing=false;
const char *fileName(){const char *p=getenv("PSU_SETTINGS_FILE");return p?p:"settings.eeprom";}
int readBytes(uint16_t address,uint8_t *data,uint16_t size,void*) {
    memset(data,0xff,size);
    FILE *f=fopen(fileName(),"rb");if(!f)return errno==ENOENT?1:-1;
    int ok=fseek(f,address,SEEK_SET)==0;
    if(ok){(void)fread(data,1,size,f);ok=!ferror(f);}
    fclose(f);return ok?1:-1;
}
int writeBytes(uint16_t address,uint8_t *data,uint16_t size,void*) {
    FILE *f=fopen(fileName(),"r+b");if(!f)f=fopen(fileName(),"w+b");if(!f)return -1;
    int ok=fseek(f,address,SEEK_SET)==0 && fwrite(data,1,size,f)==size && fflush(f)==0;
    if(fclose(f))ok=0;
    return ok?1:-1;
}
}
#else
#include "board_settings.h"
#endif
namespace ui {
void pollSettingsPersistence() {
#ifdef SIMULATOR
    if(!initialized) {
        initialized=true;
        testing=getenv("PSU_THEME_SELFTEST")||getenv("PSU_FINGER_SELFTEST")||getenv("PSU_USB_SELFTEST")||getenv("PSU_BATTERY_SELFTEST");
        psu_nv_init(&storage,readBytes,writeBytes,0);
        if(!testing && psu_nv_load(&storage,payload)==1 && !psu_config_decode(payload,sizeof(payload)))storage.loaded=0;
    }
    clockMs+=16;
    if(!testing && psu_config_encode(payload,sizeof(payload))) {
        psu_nv_request(&storage,payload,clockMs);psu_nv_tick(&storage,clockMs);
    }
#else
    BoardSettings_Poll(HAL_GetTick());
#endif
}
const char *settingsPersistenceStatus() {
#ifdef SIMULATOR
    return storage.failed?"Settings write failed":storage.dirty||storage.state?"Saving settings...":"Settings saved";
#else
    return !BoardSettings_diag.available?"EEPROM unavailable":BoardSettings_diag.failed?"Settings write failed":
        BoardSettings_diag.pending?"Saving settings...":"Settings saved";
#endif
}
}
