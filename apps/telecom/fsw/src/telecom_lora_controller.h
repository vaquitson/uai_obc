#ifndef TELECOM_CONTROLER_H
#define TELECOM_CONTROLER_H

#include <stdlib.h>
#include "osapi.h"

//#define SEND_FREQ "435.500"

/* Timeout per OSAL I/O operation, including frequency commands. */
#define LORA_CONTROLLER_IO_TIMEOUT_MS 1000

#define RECV_FREQ "435.500"
#define SEND_FREQ "435.500"

// Controller mesage to set frequency
#define SET_RECV_FREQ_CMD "FREQ:"RECV_FREQ"\n\0"
#define SET_SEND_FREQ_CMD "FREQ:"SEND_FREQ"\n\0"

#define CONTROLLER_SATE_INVALID 0
#define CONTROLLER_SATE_RECV    1
#define CONTROLLER_SATE_SEND    2

#define LORA_CONTROLLER_SUCCESS            0
#define LORA_CONTROLLER_WRITING_ERR       -1
#define LORA_CONTROLLER_FD_ERR            -2
#define LORA_CONTROLLER_NULL_PTR_ERR      -3
#define LORA_CONTROLLER_FREQ_ERR          -4

typedef struct {
  osal_id_t fd;
  int state;
} LoraController;

void lora_controller_init(LoraController *con);

int32 lora_controller_send(LoraController *cont, const char *str, size_t str_len);

int32 lora_controller_recv(LoraController *cont, char *buf, size_t buf_size, int *err);

/* fd must be a handle opened through OSAL, not a POSIX file descriptor. */
void lora_controller_set_fd(LoraController *con, osal_id_t fd);
osal_id_t lora_controller_get_fd(LoraController *con);

#endif
