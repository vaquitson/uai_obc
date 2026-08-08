/**
* @file narwal_cpu.c
* @brief Internal implementation of the Narwal CPU interface.
*
* This file contains the functions required to retrieve and process
* CPU information from the /proc/stat file.
*
* For more information about the /proc/stat format, see:
* https://docs.kernel.org/filesystems/proc.html#miscellaneous-kernel-statistics-in-proc-stat
*
* @author Narwal
*/

#include <stddef.h>
#include <fcntl.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
 
#include "narwal_cpu.h"

int stat_fd = -1;

//###### UTILITIS  ######//

#define NARWAL_PRIV_OPT_NO_RESET_SEEK 1
#define NARWAL_PRIV_OPT_RESET_SEEK 2

#define NARWAL_CPU_EVENT_PREFIX(X) NARWAL_CPU_##X

#define NARWAL_CPU_USER_TIME         1  /* Time spent executing user processes. */
#define NARWAL_CPU_NICE_TIME         2  /* Time spent executing niced user processes. */
#define NARWAL_CPU_SYSTEM_TIME       3  /* Time spent executing kernel processes. */
#define NARWAL_CPU_IDLE_TIME         4  /* Time spent idle. */
#define NARWAL_CPU_IOWAIT_TIME       5  /* Time spent waiting for I/O completion. */
#define NARWAL_CPU_IRQ_TIME          6  /* Time spent servicing hardware interrupts. */
#define NARWAL_CPU_SOFTIRQ_TIME      7  /* Time spent servicing software interrupts. */
#define NARWAL_CPU_STEAL_TIME        8  /* Time involuntarily waiting for other virtual CPUs. */
#define NARWAL_CPU_GUEST_TIME        9  /* Time spent running a guest VM. */
#define NARWAL_CPU_GUEST_NICE_TIME  10  /* Time spent running a niced guest VM. */

#define NARWAL_CPU_STAT_PATH "/proc/stat"
#define NARWAL_CPU_READ_SIZE 1024

// Initialize the global fd if not priviously intialize
static ssize_t priv_narwal_string_read_fd(
        int *fd, 
        char *buf, size_t size, 
  const char *path,
  unsigned int opt, int *err)
{

  ssize_t data_read;

  if (buf == NULL) {
    *err = NARWAL_CPU_EVENT_PREFIX(NULL_BUF);
    return -1;
  }

  if (*fd < 0) {
    *fd = open(path , O_RDONLY);
    if (*fd < 0){
      *err = NARWAL_CPU_EVENT_PREFIX(OPEN_ERR);
      return -1;
    }
  }

  data_read = read(*fd, buf, size-1);
  if (data_read < 0){
    *err = NARWAL_CPU_EVENT_PREFIX(READ_ERR);
    return -1;
  }

  buf[data_read] = '\0';

  if ((opt & NARWAL_PRIV_OPT_RESET_SEEK) == NARWAL_PRIV_OPT_RESET_SEEK)
    if (lseek(*fd, 0, SEEK_SET) == -1){
      *err = NARWAL_CPU_EVENT_PREFIX(SEEK_RESET_ERR);
      return -1; 
    }
   
  return data_read;
}


// from https://raw.githubusercontent.com/vaquitson/narwal_algorithms/refs/heads/main/string_parsing/src/narwal_column_parsing.c
static int narwal_str_get_col(
  const char *str_p,   size_t str_l, 
  const char *delim_p, size_t delim_l,
  char *buf_p,   size_t buf_s,
  int    index) 
{
  size_t start = 0;
  size_t end = 0;
  size_t add = 0; 
  size_t buf_l = 0;
  
  int data_counter = -1;
  
  if (str_l == 0 || delim_l == 0)
    return -2;
  
  end = delim_l;

  while(start < str_l) {
    if (strncmp(str_p+start, delim_p, delim_l) == 0) {
      if (buf_l > 0) {
        buf_p[buf_l] = '\0';
        buf_l = 0;
        data_counter++;

        if (data_counter == index)
          return 0;
      }

      add = delim_l;
    } else {

      if (str_p[start] == '\n')
        break;

      if (buf_l < buf_s)
        buf_p[buf_l] = str_p[start];

      add = 1;
      buf_l++;
    }

    start += add;
    end   += add;

    if (end > str_l)
      end = start;
  }

  if (buf_l > 0){

    if (data_counter + 1 == index){
      buf_p[buf_l] = '\0';
    }
    return 0;
  }

  return -1; 
}

//###### END UTILITIS  ######//


/**
* @brief Fill a buffer with the information
* of the provided cpu number
*
* This fcuntions fills an array "val_arr_p" of len "val_arr_l"
* with the information corresponding to the cpu withe the 
* number "cpu_n"
*
* @param val_arr_p A pointer to an array of NarwalCpuTime_t
* @param val_arr_l The number of elements the "val_arr_p" coudl
* hold
* @param cpu_n The number of the cpu you want the information
*
* For more information: https://docs.kernel.org/filesystems/proc.html#miscellaneous-kernel-statistics-in-proc-stat
*
* @retval NARWAL_CPU_KEY_NOT_FOUND The "cpu_n" was not found 
* @retval NARWAL_CPU_READ_ERR Something went wrong while trying to read
* the /proc/stat file
*/
static int priv_narwal_cpu_get_info(
  NarwalCpuTime_t *val_arr_p, size_t val_arr_l, int cpu_n)
{
  char cpu_name_str_p[5] = {"cpu"};

  char number_str_p[30] = {0};

  char stat_str_p[NARWAL_CPU_READ_SIZE] = {0};
  char *stat_str_cpu_info_p = NULL;

  int rc = 0;
  size_t i = 0;
  int err = 0;

  ssize_t read_size = 0;

  if (val_arr_l > 10)
    val_arr_l = 10;
  
  // Esta restriccion se puede relajar
  if (cpu_n > 9 || cpu_n < -1){
    return NARWAL_CPU_EVENT_PREFIX(KEY_NOT_FOUND);
  }

  if (cpu_n == NARWAL_CPU_GENERAL_CPU) {
    cpu_name_str_p[3] = '\0';
  } else { 
    cpu_name_str_p[3] = 48 + cpu_n;
  }

  cpu_name_str_p[4] = '\0';

  // bastante fucking risky
  while (1) { 
    read_size = priv_narwal_string_read_fd(&stat_fd, stat_str_p, 
                                           NARWAL_CPU_READ_SIZE, NARWAL_CPU_STAT_PATH, 
                                           NARWAL_PRIV_OPT_NO_RESET_SEEK, &err);

    if (read_size < 0)
      return NARWAL_CPU_EVENT_PREFIX(READ_ERR);

    stat_str_cpu_info_p = strstr(stat_str_p, cpu_name_str_p);
    if (stat_str_cpu_info_p != NULL){
      break;
    }

    if (read_size == 0) {
      return NARWAL_CPU_EVENT_PREFIX(KEY_NOT_FOUND);
    }
  };
  
  for (i = 0; i < val_arr_l; i++) {
    rc = narwal_str_get_col(
      stat_str_cpu_info_p, (read_size - (size_t)(stat_str_p - stat_str_cpu_info_p)), 
      " ", 1, 
      number_str_p, 30,
      i+1);

    if (rc < 0){
      return NARWAL_CPU_EVENT_PREFIX(KEY_NOT_FOUND);
    }

    val_arr_p[i] = strtol(number_str_p, NULL, 10);
  }

  lseek(stat_fd, 0, SEEK_SET);
  return i;
}


int narwal_cpu_init(NarwalCpu *cpu_p, int cpu_n){
  if (cpu_p == NULL)
    return NARWAL_CPU_EVENT_PREFIX(NULL_PTR);

  if (cpu_n > 9 || cpu_n < -1)
    return NARWAL_CPU_EVENT_PREFIX(OUT_OF_BOUNCE); 

  cpu_p->cpu_n = cpu_n;

  return NARWAL_CPU_EVENT_PREFIX(SUCCESS);
}


NarwalCpuTime_t narwal_cpu_busy_time(NarwalCpu *cpu_p){
  static const int narwal_cpu_busy_time_keys[] = {
    NARWAL_CPU_USER_TIME,
    NARWAL_CPU_NICE_TIME,
    NARWAL_CPU_SYSTEM_TIME,
    NARWAL_CPU_IRQ_TIME,
    NARWAL_CPU_SOFTIRQ_TIME,
    NARWAL_CPU_STEAL_TIME
  };

  int rc = 0;
  NarwalCpuTime_t total_time = 0;
  if (cpu_p == NULL) 
    return NARWAL_CPU_EVENT_PREFIX(NULL_PTR);

  rc = priv_narwal_cpu_get_info(
    cpu_p->info_arr, 10, cpu_p->cpu_n);


  if (rc != 10)
    return rc;

  for (size_t i = 0; i < sizeof(narwal_cpu_busy_time_keys)/sizeof(int); i++){
    total_time += cpu_p->info_arr[narwal_cpu_busy_time_keys[i]-1]; 
  }

  return total_time;
}


NarwalCpuTime_t narwal_cpu_idle_time(NarwalCpu *cpu_p){
  static const int narwal_cpu_idle_time_keys[] = {
    NARWAL_CPU_IDLE_TIME,
    NARWAL_CPU_IOWAIT_TIME
  };

  int rc = 0;
  NarwalCpuTime_t total_time = 0;
  if (cpu_p == NULL) 
    return NARWAL_CPU_EVENT_PREFIX(NULL_PTR);

  rc = priv_narwal_cpu_get_info(
    cpu_p->info_arr, 10, cpu_p->cpu_n);


  if (rc != 10)
    return rc;

  for (size_t i = 0; i < sizeof(narwal_cpu_idle_time_keys)/sizeof(int); i++){
    total_time += cpu_p->info_arr[narwal_cpu_idle_time_keys[i]-1]; 
  }

  return total_time;
}


int narwal_cpu_print_info(NarwalCpu *cpu_p){
  static const char *cpu_time_names_p[] = {
    "user",
    "nice",
    "system",
    "idle",
    "iowait",
    "irq",
    "softirq",
    "steal",
    "guest",
    "guest_nice"
  };

  int rc = 0;

  if (cpu_p == NULL)
    return NARWAL_CPU_EVENT_PREFIX(NULL_PTR);

  rc = priv_narwal_cpu_get_info(
    cpu_p->info_arr, 10, cpu_p->cpu_n);

  if (rc != 10)
    return rc;

  if (cpu_p->cpu_n != NARWAL_CPU_GENERAL_CPU){
    printf("cpu %d\n", cpu_p->cpu_n);
  } else {
    printf("cpu\n");
  }

  for (int i = 0; i < 10; i++) {
    printf("\t%s: %lld\n",
           cpu_time_names_p[i],
           (long long)cpu_p->info_arr[i]);
  }

  return NARWAL_CPU_EVENT_PREFIX(SUCCESS);
}


// Esta funcion podria no estar.
int narwal_cpu_info(NarwalCpu *cpu_p) {
  int rc = 0;

  rc = priv_narwal_cpu_get_info(
    cpu_p->info_arr, 10, cpu_p->cpu_n);

  if (rc != 10){
    return rc;
  }
  
   return NARWAL_CPU_EVENT_PREFIX(SUCCESS);
}


#ifdef NARWAL_TEST
int *priv_narwal_cpu_get_fd()
{
  return &stat_fd;
}
#endif
