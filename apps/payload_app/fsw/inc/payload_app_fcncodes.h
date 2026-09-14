#ifndef PAYLOAD_APP_FCNCODES_H
#define PAYLOAD_APP_FCNCODES_H

/** No-op, only bumps the command counter and issues an event */
#define PAYLOAD_APP_NOOP_CC 0

/** Reset the command/error counters */
#define PAYLOAD_APP_RESET_COUNTERS_CC 1

/** Open, read and parse the CSV file named in the command payload */
#define PAYLOAD_APP_READ_FILE_CC 2

#endif
