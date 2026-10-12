#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
 const char *mode=getenv("BOARD_MOCK_CASE"), *state=getenv("BOARD_MOCK_STATE");
 int display=0, changed=0;
 for(int i=1;i<argc;i++) if(!strcmp(argv[i],"displ")) display=1;
 FILE *f=fopen(state,"r"); if(f){changed=1;fclose(f);}
 if(!strcmp(mode,"connection_fail")) {puts("Error: No debug probe detected."); return 1;}
 if(display) {
  printf("Device ID : 0x%s\n", !strcmp(mode,"wrong_mcu")?"468":"485");
  int good=(!strcmp(mode,"unchanged") || (changed && strcmp(mode,"readback_fail")));
  printf("XSPI1_HSLV : 0x%d\nXSPI2_HSLV : 0x%d\nI2C_NI3C : 0x%d\nIWDG_HW : 0x%d\nVDDIO_HSLV : 0x%d\n",good?1:0,good?0:1,good?1:0,good?1:0,good?0:1);
  return 0;
 }
 if(!strcmp(mode,"write_fail")) return 1;
 f=fopen(state,"w"); if(!f) return 2;
 for(int i=1;i<argc;i++) if(strchr(argv[i],'=')) fprintf(f,"%s\n",argv[i]);
 fclose(f); return 0;
}