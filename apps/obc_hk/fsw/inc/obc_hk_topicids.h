/**
 * @file
 *   OBC_HK Application Topic IDs
 */
#ifndef OBC_HK_TOPICIDS_H
#define OBC_HK_TOPICIDS_H

#include "obc_hk_topicid_values.h"

#define DEFAULT_OBC_HK_MISSION_CMD_TOPICID     0x82
#define DEFAULT_OBC_HK_MISSION_HK_TLM_TOPICID  0x83

// valores publicos
#define OBC_HK_MISSION_CMD_TOPICID             OBC_HK_MISSION_TIDVAL(CMD)
#define OBC_HK_MISSION_HK_TLM_TOPICID          OBC_HK_MISSION_TIDVAL(HK_TLM)

#endif
