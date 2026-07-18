#ifndef NARWAL_THERMAL_ZONE_H
#define NARWAL_THERMAL_ZONE_H

#include <dirent.h>
#include <stddef.h>


// configs
#define NARWAL_THERMAL_ZONES_CONFIG_H

#define NARWAL_THERMAL_ZONE_PATH_MAX 100
#define NARWAL_THERMAL_ZONE_THERMAL_DIR "/sys/class/thermal/"

#define NARWAL_THERMAL_ZONE_TYPE_MAX 20
#define NARWAL_THERMAL_ZONE_TEMP_MAX 6
#define NARWAL_THERMAL_ZONE_TEMPERATURE_CELCIUS
// end configs

typedef struct  {
  char path[NARWAL_THERMAL_ZONE_PATH_MAX];
  size_t path_len;
  char type[NARWAL_THERMAL_ZONE_TYPE_MAX]; 
  size_t type_len;
  int error;
} NarwalThermalZone;

//private functions
int priv_narwal_thermal_zones_get_next(NarwalThermalZone *tz_p, unsigned int opts);
//end private functions

char *narwal_thermal_zones_get_type(NarwalThermalZone *tz_p);
float narwal_thermal_zones_get_temp(NarwalThermalZone *tz_p);

/*
 * Retrieves the next thermal zone available on the system.
 *
 * On each successful call, this function clears the structure pointed to by
 * `tz_p` and fills it with information about the next available thermal zone.
 * Successive calls iterate through all thermal zones detected on the system.
 *
 * @param tz_p  Pointer to the structure in which the thermal zone information
 *              will be stored.
 *
 * @return A status code indicating whether a thermal zone was retrieved,
 *         whether the iteration has finished, or whether an error occurred.
*/
#define narwal_thermal_zones_get_next(tz_p)  priv_narwal_thermal_zones_get_next((tz_p), 0)


int narwal_thermal_zones_get_by_type(NarwalThermalZone *tz_p, const char *type);

enum NARWAL_THERMAL_ZONE_RC {
  NARWAL_THERMAL_ZONE_ERR,
  NARWAL_THERMAL_ZONE_SUCESS,
  // there are no more thermal zones left
  NARWAL_THERMAL_ZONE_NO_MORE_TZ,
  NARWAL_THERMAL_ZONE_TZ_DOES_NOT_EXIST,
};

enum NARWAL_THERMAL_ZONE_ERRS {
  NARWAL_THERMAL_ZONE_INVALID_PATH,
};


#endif
