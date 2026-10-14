/**
 * @file
 *   PAYLOAD_APP Application Topic IDs
 */
#ifndef DEFAULT_PAYLOAD_APP_MSGID_VALUES_H
#define DEFAULT_PAYLOAD_APP_MSGID_VALUES_H

#include "cfe_core_api_base_msgids.h"

#include "../fsw/inc/payload_app_topicids.h"

#define PAYLOAD_APP_CMD_PLATFORM_MIDVAL(x) CFE_PLATFORM_CMD_TOPICID_TO_MIDV(PAYLOAD_APP_MISSION_##x##_TOPICID)
#define PAYLOAD_APP_TLM_PLATFORM_MIDVAL(x) CFE_PLATFORM_TLM_TOPICID_TO_MIDV(PAYLOAD_APP_MISSION_##x##_TOPICID)

#endif
