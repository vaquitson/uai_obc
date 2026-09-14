/**
 * @file
 *   PAYLOAD_APP Application Platform Configuration
 */
#ifndef DEFAULT_PAYLOAD_APP_PLATFORM_CFG_H
#define DEFAULT_PAYLOAD_APP_PLATFORM_CFG_H

/** Depth of the command pipe */
#define PAYLOAD_APP_PIPE_DEPTH 16

/** Timeout (ms) used while waiting for a command on the pipe, between HK sends */
#define PAYLOAD_APP_SB_TIMEOUT_MS 1000

/** Max length (incl. null) of a CSV file path passed in a command */
#define PAYLOAD_APP_FILENAME_LEN 64

/** Size of the in-memory buffer used to read a CSV file in one shot */
#define PAYLOAD_APP_FILE_BUF_SIZE 4096

/** Max number of data rows parsed from a CSV file */
#define PAYLOAD_APP_MAX_ROWS 50

/** Max number of numeric columns parsed per row */
#define PAYLOAD_APP_MAX_COLS 16

/** CSV file read automatically at app startup */
#define PAYLOAD_APP_DEFAULT_FILE "/cf/payload_app_data.csv"

/** Pausa (ms) entre cada relectura/impresion periodica del CSV */
#define PAYLOAD_APP_READ_INTERVAL_MS 5000

#endif
