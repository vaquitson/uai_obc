#ifndef TELECOM_TLM_H
#define TELECOM_TLM_H

#include "cfe_error.h"

void TELECOM_forward_tlm(void);

/*
 * Implementation dependent
 * Initialize the application telemetry
 *
 * The meaning of the ptr depends on the implementation
 *
 */
CFE_Status_t TELECOM_open_tlm(const void *ptr);

#endif
