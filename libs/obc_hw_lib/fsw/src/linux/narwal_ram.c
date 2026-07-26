#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
 
#include "narwal_ram.h"

int meminfo_fd = -1;

MemSize_t priv_narwal_ram_parse_line(char *str){
  char buf[NARWAL_RAM_ENTRY_VALUE_MAX] = {0};
  size_t i = 0;

  while (*str != '\0' && *str != '\n'){
    if (i > NARWAL_RAM_ENTRY_VALUE_MAX-1){
      return NARWAL_RAM_TO_LARGE_ENTRY;
    }

    if (isdigit((unsigned char)*str)){
      buf[i] = *str;
      i++;
    }
    str++; 
  }
  buf[i] = '\0';

  return strtol(buf, NULL, 10);
}


MemSize_t priv_narwal_ram_get_entry_from_str(char *str, const char *key){
  char *entry_p;
  
  if (str == NULL || key == NULL)
    return NARWAL_READ_SIZE;

  entry_p = strstr(str, key);
  if (entry_p == NULL)
    return NARWAL_RAM_KEY_NOT_FOUND; 

  return priv_narwal_ram_parse_line(entry_p);
}


#define NARWAL_PRIV_OPT_RESET_SEEK 1

int priv_narwal_read_meminfo(char *buf, size_t size, unsigned int opt){
  if (buf == NULL)
    return NARWAL_RAM_NULL_BUF;

  ssize_t data_read;

  if (meminfo_fd < 0) {
    meminfo_fd = open(NARWAL_RAM_MEMINFO_PATH, O_RDONLY);
    if (meminfo_fd < 0)
      return NARWAL_RAM_OPEN_ERR; 
  }

  data_read = read(meminfo_fd, buf, size-1);
  if (data_read < 0)
    return NARWAL_RAM_READ_ERR;
  buf[data_read] = '\0';


  if ((opt & NARWAL_PRIV_OPT_RESET_SEEK) == NARWAL_PRIV_OPT_RESET_SEEK){
    if (lseek(meminfo_fd, 0, SEEK_SET) == -1)
      return NARWAL_RAM_SEEK_RESET_ERR;
  }
  return data_read;
}



MemSize_t narwal_ram_size(void){
  static long int ram_size = 0;
  
  if (ram_size == 0){
    long int rc;
    char buf[NARWAL_READ_SIZE];
    ssize_t data_read;

    data_read = priv_narwal_read_meminfo(buf, NARWAL_READ_SIZE, NARWAL_PRIV_OPT_RESET_SEEK);
    if (data_read < 0)
      return data_read;

    ram_size = priv_narwal_ram_get_entry_from_str(buf, "MemTotal");
    if (ram_size < 0){
      rc = ram_size;
      ram_size = 0;
      return rc;
    }

    if (lseek(meminfo_fd, 0, SEEK_SET) == -1)
      return NARWAL_RAM_SEEK_RESET_ERR;
  }
  return ram_size;
}


MemSize_t narwal_ram_usage(void){
  static char buf[NARWAL_READ_SIZE] = {0};

  ssize_t data_read;
  long int ram_free = 0;
  long int ram_size = 0;

  data_read = priv_narwal_read_meminfo(buf, NARWAL_READ_SIZE, NARWAL_PRIV_OPT_RESET_SEEK);
  if (data_read < 0)
    return data_read;

  ram_free = priv_narwal_ram_get_entry_from_str(buf, "MemFree");
  if (ram_free < 0)
    return ram_free;

  ram_size = narwal_ram_size();
  if (ram_size < 0)
    return ram_size;

  return ram_size - ram_free;
}


float narwal_ram_usage_percentage(){
  MemSize_t ram_size;  
  MemSize_t ram_usgae;

  ram_size = narwal_ram_size();
  if (ram_size < 0)
    return ram_size;

  ram_usgae = narwal_ram_usage();
  if (ram_usgae < 0)
    return ram_usgae;

  return ((100.0 * ram_usgae) / ram_size);
}

