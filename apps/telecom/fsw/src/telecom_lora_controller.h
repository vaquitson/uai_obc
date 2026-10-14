#ifndef TELECOM_CONTROLER_H
#define TELECOM_CONTROLER_H

#include <stdlib.h>
#include "osapi.h"

//#define SEND_FREQ "435.500"

/* Timeout per POSIX I/O operation, including frequency commands. */
#define LORA_CONTROLLER_IO_TIMEOUT_MS 1000

#define RECV_FREQ "435.500"
#define SEND_FREQ "435.500"

// Controller mesage to set frequency
#define SET_RECV_FREQ_CMD "FREQ:"RECV_FREQ"\n\0"
#define SET_SEND_FREQ_CMD "FREQ:"SEND_FREQ"\n\0"

#define CONTROLLER_SATE_INVALID 0
#define CONTROLLER_SATE_RECV    1
#define CONTROLLER_SATE_SEND    2

#define LORA_CONTROLLER_SUCCESS             0
#define LORA_CONTROLLER_WRITING_ERR        -1
#define LORA_CONTROLLER_FD_ERR             -2
#define LORA_CONTROLLER_NULL_PTR_ERR       -3
#define LORA_CONTROLLER_FREQ_ERR           -4
#define LORA_CONTROLLER_FREQ_IS_NOT_SET    -5
#define LORA_CONTROLLER_NO_CONFIRMATION    -6
#define LORA_CONTROLLER_RX_MSG_NOR_RECIVED -7

typedef struct {
  int fd; /* POSIX descriptor; -1 when not open. */
  int state;
  char downlink_freq[10];
  char uplink_freq[10];
} LoraController;


/**
 * Initializes the LoRa Controller
 * Does not set the frequencys at wich send and
 * recive commands.
*/
int lora_controller_init(LoraController *cont, const char *path);

/**
 * Set the uplink frequency of the controller.
*/
void lora_controller_set_uplink_freq(LoraController *cont, const char *freq);

/** 
 * Set the downlik frequency for the controller
*/
void lora_controller_set_downlik_freq(LoraController *cont, const char *freq);

/**
 * Get the uplink frequency currently beeing used in the controller
 */
const char *lora_controller_get_uplink_freq(const LoraController *con);

/**
 * Get the downlik frequency currently beeing used in the controller
 */
const char *lora_controller_get_downlink_freq(const LoraController *con);


/**
 * @brief Write a payload, configuring SEND_FREQ first if necessary.
 *
 * When the cached state is not CONTROLLER_SATE_SEND, writes the frequency
 * command and updates the state on success. Then performs one payload write.
 * Each I/O operation has its own 1000 ms timeout; switching frequency and
 * writing the payload can therefore require two separate waits.
 *
 * @param[in,out] cont Controller with an open POSIX descriptor.
 * @param[in] payload Data to send; need not be null-terminated.
 * @param[in] payload_len Number of bytes to write; must be greater than zero.
 * @return Nonnegative number of bytes written, which may be less than
 *         payload_len; partial writes are not retried. Returns -1 for NULL
 *         arguments or frequency setup failure. POSIX I/O failures map to
 *         OS_ERROR, and payload write timeouts map to OS_ERROR_TIMEOUT.
 */
int32 lora_controller_send(LoraController *cont, const char *str, size_t str_len);

/**
 * @brief Read available data, configuring RECV_FREQ first if necessary.
 *
 * When the cached state is not CONTROLLER_SATE_RECV, writes the frequency
 * command and updates the state on success. Then performs one read without
 * waiting for the entire buffer to fill or adding a string terminator.
 * Each I/O operation has its own 1000 ms timeout; switching frequency and
 * reading data can therefore require two separate waits.
 *
 * @param[in,out] cont Controller with an open POSIX descriptor.
 * @param[out] buf Destination buffer for the received bytes.
 * @param[in] buf_size Buffer capacity in bytes; must be greater than zero.
 * @param[out] err Required error pointer; set on frequency/prefix errors.
 * @return Positive number of payload bytes read, or a negative value on
 *         invalid arguments, I/O error, timeout, or end of stream.
 */
int32 lora_controller_recv(LoraController *cont, char *buf, size_t buf_size, int *err);

/* Assigned descriptors must be nonblocking to preserve I/O timeouts. */
void lora_controller_set_fd(LoraController *con, int fd);

const char *lora_controller_get_uplink_freq(const LoraController *con);

const char *lora_controller_get_downlink_freq(const LoraController *con);

/*
 * Check if the downlik frequency is set;
 */
bool lora_controller_downlink_freq_is_set(const LoraController *cont);

/**
 * check if the uplink frequecy is set
*/
bool lora_controller_uplink_freq_is_set(const LoraController *cont);

int lora_controller_get_fd(LoraController *con);

#endif
