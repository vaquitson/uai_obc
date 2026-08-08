#ifndef NARWAL_CPU_H
#define NARWAL_CPU_H

#include <stdint.h>
#include <stddef.h>

#define NarwalCpuTime_t int64_t  /**< @bief The type in which cpu time will be stored.*/

#define NARWAL_CPU_GENERAL_CPU -1 /**< identifies the acumulated cpu information*/

#define NARWAL_CPU_SUCCESS         0
#define NARWAL_CPU_OPEN_ERR       -1
#define NARWAL_CPU_READ_ERR       -2
#define NARWAL_CPU_KEY_NOT_FOUND  -3
#define NARWAL_CPU_TO_LARGE_ENTRY -4
#define NARWAL_CPU_NULL_BUF       -5
#define NARWAL_CPU_NULL_PTR       -6
#define NARWAL_CPU_SEEK_RESET_ERR -7
#define NARWAL_CPU_OUT_OF_BOUNCE  -8  /* Indicate thath some number is out of a valid range */

/**
 * @brief Stores CPU identification and accumulated CPU time information.
 *
 * This structure contains the information required to interact with the
 * Narwal CPU interface. It identifies a logical CPU and stores its accumulated
 * time counters.
*/
typedef struct{
  int cpu_n;                     /**< Logical CPU number. A value of -1 represents the aggregate CPU. */
  NarwalCpuTime_t info_arr[10];  /**< Array containing the accumulated time counters for the selected CPU. */
} NarwalCpu;


/**
* @brief Print the time information of the provided NarwalCpu structure
*
* Print the accumulated CPU time information stored in the structure pointed
* to by "cpu_p".
*
* @param cpu_p Pointer to the NarwalCpu structure containing the CPU
* information to be printed.
*
* @return A non-negative status code on success or a negative status code
* if an error occurs.
*
* @retval NARWAL_CPU_SUCCESS The CPU information was printed successfully.
* @retval NARWAL_CPU_KEY_NOT_FOUND The CPU identified by "cpu_n" was not found.
* @retval NARWAL_CPU_READ_ERR An error occurred while trying to read the CPU
* information.
*/
int narwal_cpu_print_info(NarwalCpu *cpu_p);

NarwalCpuTime_t narwal_cpu_idle_time(NarwalCpu *cpu_p);

NarwalCpuTime_t narwal_cpu_busy_time(NarwalCpu *cpu_p);

/**

* @brief Initialize the NarwalCpu structure provided by
* the "cpu_p" pointer.
*
* @param cpu_p A pointer to a NarwalCpu structure.
* @param cpu_n Number of the logical CPU we want to represent.
* The macro NARWAL_CPU_GENERAL_CPU identifies the accumulated
* information for all the CPU cores.
*
* @return On success, return a non-negative value; on error, return
* a negative value.
*
* @retval NARWAL_CPU_SUCCESS The structure was filled successfully.
* @retval NARWAL_CPU_KEY_NOT_FOUND Couldn't find a CPU with the logical number provided
* by "cpu_n".
* @retval NARWAL_CPU_NULL_PTR The "cpu_p" pointer is NULL.
*/
int narwal_cpu_init(NarwalCpu *cpu_p, int cpu_n);

/**
* @brief Returns the file descriptor for the CPU statistics file.
*
* This function is intended for testing purposes only.
*
* It returns the file descriptor used internally by most functions
* in the Narwal CPU interface to access CPU statistics.
*
* @return The file descriptor for the CPU statistics file.
*/
#ifdef NARWAL_TEST
int *priv_narwal_cpu_get_fd();
#endif


#endif
