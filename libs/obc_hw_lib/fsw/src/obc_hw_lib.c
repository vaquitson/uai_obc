#include "cfe_config.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "cfe_evs.h"


float OBC_HW_LIB_get_cpu_temp(void);

#ifdef OBC_PLATFORM_LINUX
float OBC_HW_LIB_get_cpu_temp(void){
  #include "narwal_thermal_zones.h"

  static NarwalThermalZone tz = {0};
  static NarwalThermalZone *tz_p = NULL;
  int rc;

  if (tz_p == NULL){
    tz_p = &tz;
    rc = narwal_thermal_zones_get_by_type(tz_p, "x86_pkg_temp");
    if (rc != NARWAL_THERMAL_ZONE_SUCESS)
      return rc;
  }

  return narwal_thermal_zones_get_temp(tz_p);
}

#else
float OBC_HW_LIB_get_cpu_temp(void){
  printf("SAD from cpu temp\n");
  return 0;
}

#endif


int32 OBC_HW_LIB_Init(void){
  printf("Hello from OBC_HW_LIB\n");
  printf("cpu temp: %f\n", OBC_HW_LIB_get_cpu_temp());
  return 0;
}
