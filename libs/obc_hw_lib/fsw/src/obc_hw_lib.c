#include "cfe_config.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "cfe_evs.h"

int32 OBC_HW_LIB_get_cpu_temp(void);

#ifdef OBC_PLATFORM_LINUX
int32 OBC_HW_LIB_get_cpu_temp(void){
  #include "narwal_thermal_zones.h"
  static NarwalThermalZone n_tz = {0};
  n_tz.error = 1;
  printf("HELLO form thermal zones %d\n", n_tz.error);
  return 0;
}

#else
int32 OBC_HW_LIB_get_cpu_temp(void){
  printf("SAD from cpu temp\n");
  return 0;
}

#endif


int32 OBC_HW_LIB_Init(void){
  printf("Hello from OBC_HW_LIB\n");
  OBC_HW_LIB_get_cpu_temp();
  return 0;
}
