/**
 * @file
 *   CSV_READER Application Topic IDs
 */
#ifndef DEFAULT_CSV_READER_MSGID_VALUES_H
#define DEFAULT_CSV_READER_MSGID_VALUES_H

#include "cfe_core_api_base_msgids.h"

#include "../fsw/inc/csv_reader_topicids.h"

#define CSV_READER_CMD_PLATFORM_MIDVAL(x) CFE_PLATFORM_CMD_TOPICID_TO_MIDV(CSV_READER_MISSION_##x##_TOPICID)
#define CSV_READER_TLM_PLATFORM_MIDVAL(x) CFE_PLATFORM_TLM_TOPICID_TO_MIDV(CSV_READER_MISSION_##x##_TOPICID)

#endif
