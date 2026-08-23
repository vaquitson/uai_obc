#ifndef CMD_HAND_INTERNAL_CFG_H
#define CMD_HAND_INTERNAL_CFG_H


/**
 * Max depth of the command ingestion pipe
*/
#define CMD_HAND_PLAFORM_PIPE_DEPTH 32

/**
 * ground message ingestion max size
*/
#define CMD_HAND_MAX_INGEST 768

/**
 * Max amount of contiuous messages
*/
#define CMD_HAND_MAX_INGEST_PKTS 10

/**
 * Read time out of the up link socket.
*/
#define CMD_HAND_UPLINK_RECEIVE_TIMEOUT OS_CHECK


#endif
