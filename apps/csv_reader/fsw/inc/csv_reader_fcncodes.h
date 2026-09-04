#ifndef CSV_READER_FCNCODES_H
#define CSV_READER_FCNCODES_H

/** No-op, only bumps the command counter and issues an event */
#define CSV_READER_NOOP_CC 0

/** Reset the command/error counters */
#define CSV_READER_RESET_COUNTERS_CC 1

/** Open, read and parse the CSV file named in the command payload */
#define CSV_READER_READ_FILE_CC 2

#endif
