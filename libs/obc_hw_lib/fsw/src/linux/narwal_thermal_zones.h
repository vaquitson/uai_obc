#ifndef NARWAL_THERMAL_ZONE_H
#define NARWAL_THERMAL_ZONE_H

#include <dirent.h>
#include <stddef.h>
#include "narwal_thermal_zones_config.h"

typedef struct {
  char path[NARWAL_THERMAL_ZONE_PATH_MAX];
  size_t path_len;
  char type[NARWAL_THERMAL_ZONE_TYPE_MAX]; 
  size_t type_len;
  int error;
} NarwalThermalZone;


int narwal_thermal_zones_get_next(DIR *dp, NarwalThermalZone *tzp);
char *narwal_thermal_zones_get_type(NarwalThermalZone *tz_p);
float narwal_thermal_zones_get_temp(NarwalThermalZone *tz_p);

#endif
