#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "osapi.h"
#include "telecom_lora_controller.h"
#include <fcntl.h>
#include <termios.h>
#include <limits.h>
#include <poll.h>
#include <unistd.h>

#define MAX_FRQ_LEN 8

static int32 serial_port_timed_io(int fd, void *read_buf, const void *write_buf,
                                size_t size, int32 timeout_ms){
  struct pollfd poll_fd;
  ssize_t bytes;
  int rc;

  if (fd < 0 || size == 0 || size > INT32_MAX || timeout_ms < 0){
    return OS_ERROR;
  }

  poll_fd.fd = fd;
  poll_fd.events = read_buf != NULL ? POLLIN : POLLOUT;
  poll_fd.revents = 0;

  rc = poll(&poll_fd, 1, timeout_ms);

  if (rc == 0){
    return OS_ERROR_TIMEOUT;
  }

  if (rc < 0 || (poll_fd.revents & POLLNVAL) != 0 || (poll_fd.revents & poll_fd.events) == 0){
    return OS_ERROR;
  }

  if (read_buf != NULL){
    bytes = read(fd, read_buf, size);

  } else {
    bytes = write(fd, write_buf, size);
  }

  return bytes < 0 ? OS_ERROR : (int32)bytes;
}

static int32 serial_port_timed_read(int fd, void *buf, size_t size, int32 timeout_ms){
  return serial_port_timed_io(fd, buf, NULL, size, timeout_ms);
}

static int32 serial_port_timed_write(int fd, const void *buf, size_t size, int32 timeout_ms){
  return serial_port_timed_io(fd, NULL, buf, size, timeout_ms);
}

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
  fd = open(dev_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd >= 0){
    // get the current terminal configuration
    if (tcgetattr(fd, &tty) != 0){
      close(fd);
      return -1;
    }

    // disable terminal preprocesing
    cfmakeraw(&tty);

    // In and Out BAUD rate
    if (cfsetispeed(&tty, B115200) != 0 || cfsetospeed(&tty, B115200) != 0){
      close(fd);
      return -1;
    }

    // Mierda de muy bajo nivel que no tengo ni idea
    set_8n1_confg(&tty);

    if (tcsetattr(fd, TCSANOW, &tty) != 0){
      close(fd);
      return -1;
    }
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


void lora_controller_set_fd(LoraController *cont, int fd){
 cont->fd = fd;
}


int lora_controller_get_fd(LoraController *cont){
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
  if (cont == NULL)
    return LORA_CONTROLLER_NULL_PTR_ERR;

  cont->fd = -1;
  cont->state = CONTROLLER_SATE_INVALID;
  memset(cont->downlink_freq, 0, sizeof(cont->downlink_freq));
  memset(cont->uplink_freq, 0, sizeof(cont->uplink_freq));

  if (path != NULL){
    fd = serial_port_open(path);
    if (fd >= 0){
      cont->fd = fd;
    }

    return fd;    
  }

  return 0;
}


/**
 * @brief Write a frequency command using the controller's POSIX descriptor.
 *
 * Formats the command as "FREQ:<frequency>\n" and performs one timed write
 * with LORA_CONTROLLER_IO_TIMEOUT_MS (1000 ms). Partial writes are not retried.
 * Success requires the OK:FREQ_SET acknowledgement in one read.
 * The cached controller state is not changed.
 *
 * @param[in] cont Controller with an open POSIX descriptor.
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
  static const char response_buf[] = "OK:FREQ_SET";
  char buf[50] = {0};
  char res_buf[30] = {0};
  int32 bytes;
  size_t cmd_len;
  int fd;

  if (cont == NULL || freq_str == NULL){
    return LORA_CONTROLLER_NULL_PTR_ERR;
  }

  fd = lora_controller_get_fd(cont);
  if (fd < 0){
    return LORA_CONTROLLER_FD_ERR;
  }
  
  if (strlen(freq_str) <= MAX_FRQ_LEN){
    snprintf(buf, 50, "FREQ:%s\n", freq_str);
    cmd_len = strlen(buf);
    bytes = serial_port_timed_write(fd, buf, cmd_len, LORA_CONTROLLER_IO_TIMEOUT_MS);
    if (bytes >= 0 && (size_t)bytes == cmd_len){
      bytes = serial_port_timed_read(fd, res_buf, sizeof(res_buf)-1, LORA_CONTROLLER_IO_TIMEOUT_MS);
      if (bytes > 0) {
        res_buf[bytes] = '\0'; 
        if (strstr(res_buf, response_buf) != NULL){
          return LORA_CONTROLLER_SUCCESS;
        } else {
          return LORA_CONTROLLER_WRITING_ERR;
        }
      } else {
        return LORA_CONTROLLER_NO_CONFIRMATION;
      }
    } else {
      return LORA_CONTROLLER_WRITING_ERR;
    }

  } else {
    return LORA_CONTROLLER_FREQ_ERR;
  }
}


void lora_controller_set_uplink_freq(LoraController *cont, const char *freq){
  size_t len;
  len = strlen(freq); 

  if (len > 7){
    memcpy(cont->uplink_freq, freq, len+1); 
    lora_controller_set_freq(cont, freq);
  }  
}


void lora_controller_set_downlik_freq(LoraController *cont, const char *freq){
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

    serial_port_timed_write(lora_controller_get_fd(cont), "TX:", 3, LORA_CONTROLLER_IO_TIMEOUT_MS);
    bytes = serial_port_timed_write(
      lora_controller_get_fd(cont),
      payload,
      payload_len,
      LORA_CONTROLLER_IO_TIMEOUT_MS);
    serial_port_timed_write(lora_controller_get_fd(cont), "\n", 1, LORA_CONTROLLER_IO_TIMEOUT_MS);

    return bytes;
  }

  return -1;
}


int32 lora_controller_recv(LoraController *cont, 
                            char *buf, size_t buf_size, 
                             int *err){
  int rc;
  int32 bytes;
  if (err == NULL){
    return LORA_CONTROLLER_NULL_PTR_ERR;
  }

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
      
      // read the RX: bytes
      bytes = serial_port_timed_read(lora_controller_get_fd(cont), buf, 3, LORA_CONTROLLER_IO_TIMEOUT_MS);
      if (bytes != 3){
        *err = LORA_CONTROLLER_FREQ_IS_NOT_SET;
        return -1;
      }
      bytes = serial_port_timed_read(
        lora_controller_get_fd(cont),
        buf,
        buf_size-1,
        LORA_CONTROLLER_IO_TIMEOUT_MS);

      if (bytes <= 0){
        return -1;
      }

      return bytes;

    } else {
      *err = LORA_CONTROLLER_FREQ_IS_NOT_SET;
      return -1;
    }
  } else {
    return -1;
  }
}
