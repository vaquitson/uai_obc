/**
 * @file
 *   CSV_READER Application Platform Configuration
 */
#ifndef DEFAULT_CSV_READER_PLATFORM_CFG_H
#define DEFAULT_CSV_READER_PLATFORM_CFG_H

/** Depth of the command pipe */
#define CSV_READER_PIPE_DEPTH 16

/** Timeout (ms) used while waiting for a command on the pipe, between HK sends */
#define CSV_READER_SB_TIMEOUT_MS 1000

/** Max length (incl. null) of a CSV file path passed in a command */
#define CSV_READER_FILENAME_LEN 64

/** Size of the in-memory buffer used to read a CSV file in one shot */
#define CSV_READER_FILE_BUF_SIZE 4096

/** Max number of data rows parsed from a CSV file */
#define CSV_READER_MAX_ROWS 50

/** Max number of numeric columns parsed per row */
#define CSV_READER_MAX_COLS 16

/** CSV file read automatically at app startup */
#define CSV_READER_DEFAULT_FILE "/cf/csv_reader_data.csv"

/** Pausa (ms) entre cada relectura/impresion periodica del CSV */
#define CSV_READER_READ_INTERVAL_MS 5000

#endif
