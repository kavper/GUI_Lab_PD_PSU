#include "board_settings.h"
#include "board_panel.h"
#include "psu_app.h"
#include "psu_nvstore.h"
static I2C_HandleTypeDef *i2c;
static PsuNvStore storage;
static uint8_t payload[PSU_NV_PAYLOAD];
volatile BoardSettingsDiagnostics BoardSettings_diag __attribute__((aligned(32)));
static int read_bytes(uint16_t address,uint8_t *data,uint16_t size,void *user)
{
    (void)user;
    HAL_StatusTypeDef rc=HAL_I2C_Mem_Read(i2c,0xA0,address,I2C_MEMADD_SIZE_16BIT,data,size,size>32?100:5);
    return rc==HAL_OK?1:rc==HAL_BUSY?0:-1;
}
static int write_bytes(uint16_t address,uint8_t *data,uint16_t size,void *user)
{
    (void)user;
    if(size>32u || (address&31u)+size>32u)return -1;
    HAL_StatusTypeDef rc=HAL_I2C_Mem_Write(i2c,0xA0,address,I2C_MEMADD_SIZE_16BIT,data,size,5);
    return rc==HAL_OK?1:rc==HAL_BUSY?0:-1;
}
void BoardSettings_Init(I2C_HandleTypeDef *bus)
{
    i2c=bus;
    psu_nv_init(&storage,read_bytes,write_bytes,0);
    BoardSettings_diag.available=HAL_I2C_IsDeviceReady(i2c,0xA0,2,10)==HAL_OK;
    if(BoardSettings_diag.available && psu_nv_load(&storage,payload)==1) {
        BoardSettings_diag.loaded=psu_config_decode(payload,sizeof(payload));
        if(!BoardSettings_diag.loaded)storage.loaded=0;
    }
    PsuAppearance a;psu_appearance_get(&a);
    BoardBuzzer_Configure(a.sound_enabled?a.sound_volume:0,a.sound_tone,a.sound_feedback);
    BoardSettings_diag.errors=storage.errors;BoardSettings_diag.failed=storage.failed;
}
void BoardSettings_Poll(uint32_t now)
{
    if(!BoardSettings_diag.available)return;
    if(psu_config_encode(payload,sizeof(payload)))psu_nv_request(&storage,payload,now);
    psu_nv_tick(&storage,now);
    BoardSettings_diag.pending=storage.dirty||storage.state;
    BoardSettings_diag.commits=storage.commits;BoardSettings_diag.errors=storage.errors;
    BoardSettings_diag.generation=storage.generation;
    BoardSettings_diag.failed=storage.failed;
    SCB_CleanDCache_by_Addr((uint32_t *)&BoardSettings_diag,32);
}
