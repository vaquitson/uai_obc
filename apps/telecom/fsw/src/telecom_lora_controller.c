#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "osapi.h"
#include "telecom_lora_controller.h"
#include <fcntl.h>
#include <termios.h>

#define MAX_FRQ_LEN 8

void set_8n1_confg(struct termios *tty){
  tty->c_cflag &= ~PARENB;
  tty->c_cflag &= ~CSTOPB;
  tty->c_cflag &= ~CSIZE;
  tty->c_cflag |= CS8;
}

int serial_port_open(const char *dev_path){
  struct termios tty;
  int fd;
  
  // linux dependent part
  fd = open(dev_path, O_RDWR | O_NOCTTY);
  if (fd > 0){
    // get the current terminal configuration
    tcgetattr(fd, &tty);

    // disable terminal preprocesing
    cfmakeraw(&tty);

    // In and Out BAUD rate
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Mierda de muy bajo nivel que no tengo ni idea
    set_8n1_confg(&tty);

    tcsetattr(fd, TCSANOW, &tty);
    return fd;

  } else {

    return -1;
  }
}


const char *lora_controller_get_uplink_freq(const LoraController *con){
  return con->uplink_freq;
}


const char *lora_controller_get_downlink_freq(const LoraController *con){
  return con->downlink_freq;
}


void lora_controller_set_fd(LoraController *cont, osal_id_t fd){
 cont->fd = fd;
}


osal_id_t lora_controller_get_fd(LoraController *cont){
 return cont->fd;
}


bool lora_controller_uplink_freq_is_set(const LoraController *cont){
  if (strlen(lora_controller_get_uplink_freq(cont)) != 0){
    return true;
  }

  return false;
}


bool lora_controller_downlink_freq_is_set(const LoraController *cont){
  if (strlen(lora_controller_get_uplink_freq(cont)) != 0){
    return true;
  }

  return false;
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


int lora_controller_init(LoraController *cont, const char *path){
  int fd;
  if (path != NULL){
    fd = serial_port_open(path);
    if (fd > 0){
      cont->fd = fd;
    } else {
      return fd;    
    }
  }

  cont->state = CONTROLLER_SATE_INVALID;
  memset(cont->downlink_freq, 0, sizeof(cont->downlink_freq));
  memset(cont->uplink_freq, 0, sizeof(cont->uplink_freq));

  return 0;
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
 *
 * @retval LORA_CONTROLLER_SUCCESS The entire command was written.
 * @retval LORA_CONTROLLER_NULL_PTR_ERR cont or freq_str is NULL.
 * @retval LORA_CONTROLLER_FD_ERR The stored handle is undefined.
 * @retval LORA_CONTROLLER_FREQ_ERR The frequency string is too long.
 * @retval LORA_CONTROLLER_WRITING_ERR Write error, timeout, or partial write.
 *
 */
int lora_controller_set_freq(LoraController *cont, const char *freq_str){ 
  char buf[50] = {0};
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


void lora_controller_set_uplink_freq(LoraController *cont, char *freq){
  size_t len;
  len = strlen(freq); 

  if (len > 7){
    memcpy(cont->uplink_freq, freq, len+1); 
    lora_controller_set_freq(cont, freq);
  }  
}


void lora_controller_set_downlik_freq(LoraController *cont, char *freq){
  size_t len;
  len = strlen(freq); 

  if (len > 7){
    memcpy(cont->downlink_freq, freq, len+1); 
  }
}


int32 lora_controller_send(LoraController *cont, const char *payload, size_t payload_len){
  int rc;
  int32 bytes;

  if (payload != NULL && cont != NULL){
    if (lora_controller_get_state(cont) != CONTROLLER_SATE_SEND){
      rc = lora_controller_set_freq(cont, lora_controller_get_downlink_freq(cont));
      if (rc == LORA_CONTROLLER_SUCCESS){
        priv_lora_controller_set_state(cont, CONTROLLER_SATE_SEND);
      } else {
        return -1;
      }
    }

    bytes = OS_TimedWrite(
      lora_controller_get_fd(cont),
      payload,
      payload_len,
      LORA_CONTROLLER_IO_TIMEOUT_MS);

    return bytes;
  }

  return -1;
}


int32 lora_controller_recv(LoraController *cont, 
                            char *buf, size_t buf_size, 
                             int *err){
  int rc;
  int32 bytes;

  if (cont != NULL && buf != NULL && buf_size > 0){
    if (lora_controller_uplink_freq_is_set(cont)){
      if (lora_controller_get_state(cont) != CONTROLLER_SATE_RECV){
        rc = lora_controller_set_freq(cont, lora_controller_get_uplink_freq(cont));
        if (rc == LORA_CONTROLLER_SUCCESS){
          priv_lora_controller_set_state(cont, CONTROLLER_SATE_RECV);
        } else {
          *err = rc;
          return -1;
        }
      }

      bytes = OS_TimedRead(
        lora_controller_get_fd(cont),
        buf,
        buf_size,
        LORA_CONTROLLER_IO_TIMEOUT_MS);

      return bytes;

    } else {
      *err = LORA_CONTROLLER_FREQ_IS_NOT_SET;
      return -1;
    }
  } else {
    return -1;
  }
}
