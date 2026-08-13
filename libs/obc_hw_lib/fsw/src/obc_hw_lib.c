#include "cfe_config.h"
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "cfe_evs.h"

float OBC_HW_LIB_get_cpu_temp(void);
float OBC_HW_LIB_get_ram_usage_percentage(void);
long int OBC_HW_LIB_get_ram_usage(void);
float OBC_HW_LIB_get_cpu_usage(void);

#ifdef OBC_PLATFORM_LINUX
#include "narwal_thermal_zones.h"
#include "narwal_ram.h"
#include "narwal_cpu.h"
#include <unistd.h>

float OBC_HW_LIB_get_cpu_temp(void){
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


float OBC_HW_LIB_get_ram_usage_percentage(void){
  return narwal_ram_usage_percentage();
}


long int OBC_HW_LIB_get_ram_usage(void){ 
  return narwal_ram_usage();
}


float OBC_HW_LIB_get_cpu_usage(){
  NarwalCpu cpu = {0}; 

  NarwalCpuTime_t idle_time_1;
  NarwalCpuTime_t busy_time_1;

  NarwalCpuTime_t idle_time_2;
  NarwalCpuTime_t busy_time_2;

  float percentage = 0;

  idle_time_1 = narwal_cpu_idle_time(&cpu);
  busy_time_1 = narwal_cpu_busy_time(&cpu);
     
  // 500 ms
  usleep(500000);

  idle_time_2 = narwal_cpu_idle_time(&cpu);
  busy_time_2 = narwal_cpu_busy_time(&cpu);

  NarwalCpuTime_t busy_delta = busy_time_2 - busy_time_1;
  NarwalCpuTime_t idle_delta = idle_time_2 - idle_time_1;

  percentage = 100.0 * busy_delta / (busy_delta + idle_delta);

  return percentage;
}

#else
float OBC_HW_LIB_get_cpu_temp(void){
  return 0;
}

float OBC_HW_LIB_get_ram_usage_percentage(void){
  return 0;
}

long int OBC_HW_LIB_get_ram_usage(void){
  return 0;
}

float OBC_HW_LIB_get_cpu_usage(void){
  return 0;
}

#endif





int32 OBC_HW_LIB_Init(void){
  printf("Hello from OBC_HW_LIB\n");
  return 0;
}
