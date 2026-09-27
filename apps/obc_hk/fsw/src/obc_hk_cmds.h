#ifndef OBC_HK_CMDS_H
#define OBC_HK_CMDS_H

#include "cfe_sb.h"
#include "obc_hk_msg.h"

CFE_Status_t OBC_HK_noop_cmd(const OBC_HK_NoopCmd_t *data);

CFE_Status_t OBC_HK_obc_send_info_cmd(const OBC_HK_SendObcInfoCmd_t *data);
CFE_Status_t OBC_HK_send_cpu_temp_cmd(const OBC_HK_SendCpuTempCmd_t *data);
CFE_Status_t OBC_HK_send_ram_usage_cmd(const OBC_HK_SendRamUsageCmd_t *data);
CFE_Status_t OBC_HK_send_cpu_usage_cmd(const OBC_HK_SendCpuUsageCmd_t *data);

#endif
