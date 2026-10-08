#ifndef PAYLOAD_APP_WORKER_H
#define PAYLOAD_APP_WORKER_H

/*
** Socket worker child task: takes PAYLOAD_APP_Request_t items from the
** request queue and runs them one at a time against the Payload API, so the
** main task (command pipe + HK) never blocks on socket I/O.
*/
void PAYLOAD_APP_WorkerMain(void);

#endif
