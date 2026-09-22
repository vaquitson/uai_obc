#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "osapi.h"
#include "telecom_lora_controller.h"

#define MAX_FRQ_LEN 8

/**
 * @brief Initialize the controller with an undefined handle and an invalid state.
 *
 * Does not open a device or close a previously assigned handle.
 *
 * @param[out] cont Controller to initialize; must not be NULL.
 */
void lora_controller_init(LoraController *cont){
  cont->fd = -1;
  cont->state = CONTROLLER_SATE_INVALID;
}

/**
 * @brief Set the OSAL handle used for controller I/O.
 
 * @param[in,out] cont Controller to update; must not be NULL.
 * @param[in] fd OSAL handle, not a native POSIX file descriptor.
 */
void lora_controller_set_fd(LoraController *cont, osal_id_t fd){
 cont->fd = fd;
}

/**
 * @brief Return the stored OSAL handle without validating it.
 *
 * @param[in] cont Controller to query; must not be NULL.
 * @return Stored handle, or OS_OBJECT_ID_UNDEFINED after initialization.
 */
osal_id_t lora_controller_get_fd(LoraController *cont){
 return cont->fd;
}


/**
 * @brief Update the controller state without sending a device command.
 *
 * @param[in,out] cont Controller to update; must not be NULL.
 * @param[in] state Expected to be a CONTROLLER_SATE_* value; not validated here.
 */
void priv_lora_controller_set_state(LoraController *cont, int state){
 cont->state = state;
}

/**
 * @brief Return the cached state without querying the device.
 *
 * @param[in] cont Controller to query; must not be NULL.
 * @return Stored state, initially CONTROLLER_SATE_INVALID.
 */
int lora_controller_get_state(LoraController *cont){
 return cont->state;
}

/**
 * @brief Write a frequency command using the controller's OSAL handle.
 *
 * Formats the command as "FREQ:<frequency>\n" and performs one timed write
 * with LORA_CONTROLLER_IO_TIMEOUT_MS (1000 ms). Partial writes are not retried.
 * Success means the entire command was written; no device acknowledgement is
 * checked and the cached controller state is not changed.
 *
 * @param[in] cont Controller with an assigned OSAL handle.
 * @param[in] freq_str Null-terminated frequency string, at most MAX_FRQ_LEN
 *                    characters. Only its length is validated.
 * @retval LORA_CONTROLLER_SUCCESS The entire command was written.
 * @retval LORA_CONTROLLER_NULL_PTR_ERR cont or freq_str is NULL.
 * @retval LORA_CONTROLLER_FD_ERR The stored handle is undefined.
 * @retval LORA_CONTROLLER_FREQ_ERR The frequency string is too long.
 * @retval LORA_CONTROLLER_WRITING_ERR Write error, timeout, or partial write.
 *
 * @note Uses a shared static command buffer; concurrent calls must be serialized.
 */
int lora_controller_set_freq(LoraController *cont, const char *freq_str){ 
  static char buf[50] = {0};
  int32 bytes;
  size_t cmd_len;
  osal_id_t fd;

  if (cont == NULL || freq_str == NULL){
    return LORA_CONTROLLER_NULL_PTR_ERR;
  }

  fd = lora_controller_get_fd(cont);
  if (!OS_ObjectIdDefined(fd)){
    return LORA_CONTROLLER_FD_ERR;
  }
  
  if (strlen(freq_str) <= MAX_FRQ_LEN){ 
    snprintf(buf, 50, "FREQ:%s\n", freq_str);
    cmd_len = strlen(buf);
    bytes = OS_TimedWrite(fd, buf, cmd_len, LORA_CONTROLLER_IO_TIMEOUT_MS); 
    if (bytes >= 0 && (size_t)bytes == cmd_len){
      return LORA_CONTROLLER_SUCCESS; 
    } else {
      return LORA_CONTROLLER_WRITING_ERR;
    }
  } else {
    return LORA_CONTROLLER_FREQ_ERR;
  }
}


/**
 * @brief Write a payload, configuring SEND_FREQ first if necessary.
 *
 * When the cached state is not CONTROLLER_SATE_SEND, writes the frequency
 * command and updates the state on success. Then performs one payload write.
 * Each I/O operation has its own 1000 ms timeout; switching frequency and
 * writing the payload can therefore require two separate waits.
 *
 * @param[in,out] cont Controller with an assigned OSAL handle.
 * @param[in] payload Data to send; need not be null-terminated.
 * @param[in] payload_len Number of bytes to write; must be greater than zero.
 * @return Nonnegative number of bytes written, which may be less than
 *         payload_len; partial writes are not retried. Returns -1 for NULL
 *         arguments or frequency setup failure. Otherwise returns the OSAL
 *         write error, including OS_ERROR_TIMEOUT if the payload write times out.
 */
int32 lora_controller_send(LoraController *cont, const char *payload, size_t payload_len){
  int rc;
  int32 bytes;

  if (payload != NULL && cont != NULL){
    if (lora_controller_get_state(cont) != CONTROLLER_SATE_SEND){
      rc = lora_controller_set_freq(cont, SEND_FREQ);
      if (rc == LORA_CONTROLLER_SUCCESS){
        priv_lora_controller_set_state(cont, CONTROLLER_SATE_SEND);
      } else {
        return -1;
      }
    }

    bytes = OS_TimedWrite(lora_controller_get_fd(cont),
                  payload,
                  payload_len,
                  LORA_CONTROLLER_IO_TIMEOUT_MS);
    return bytes;
  }

  return -1;
}



/**
 * @brief Read available data, configuring RECV_FREQ first if necessary.
 *
 * When the cached state is not CONTROLLER_SATE_RECV, writes the frequency
 * command and updates the state on success. Then performs one read without
 * waiting for the entire buffer to fill or adding a string terminator.
 * Each I/O operation has its own 1000 ms timeout; switching frequency and
 * reading data can therefore require two separate waits.
 *
 * @param[in,out] cont Controller with an assigned OSAL handle.
 * @param[out] buf Destination buffer for the received bytes.
 * @param[in] buf_size Buffer capacity in bytes; must be greater than zero.
 * @param[in] err Unused parameter; may be NULL and is never modified.
 * @return Number of bytes read, or 0 at end of stream. Returns -1 for a NULL
 *         controller or buffer, zero buffer capacity, or frequency setup
 *         failure. Otherwise returns the OSAL read error, including
 *         OS_ERROR_TIMEOUT if no data arrives before the read timeout.
 */
int32 lora_controller_recv(LoraController *cont, 
                            char *buf, size_t buf_size, 
                             int *err){
  int rc;
  int32 bytes;

  if (cont != NULL && buf != NULL && buf_size > 0){
    if (lora_controller_get_state(cont) != CONTROLLER_SATE_RECV){
      rc = lora_controller_set_freq(cont, RECV_FREQ);
      if (rc < 0){
        return -1;
      }
      priv_lora_controller_set_state(cont, CONTROLLER_SATE_RECV);
    }

    bytes = OS_TimedRead(
      lora_controller_get_fd(cont),
      buf,
      buf_size,
      LORA_CONTROLLER_IO_TIMEOUT_MS);

    return bytes;
  }

  return -1;
}
