#ifndef PSU_NETWORK_H
#define PSU_NETWORK_H
#ifdef __cplusplus
extern "C" {
#endif
void PSU_Network_Init(void);
void PSU_Network_Process(void);
const char* PSU_Network_Address(void);
#ifdef __cplusplus
}
#endif
#endif
