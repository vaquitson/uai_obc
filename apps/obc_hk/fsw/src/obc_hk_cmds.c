#include "obc_hw_lib.h"
#include "cfe_sb.h"
#include "cfe_msg.h"
#include "obc_hk_msg.h"
#include "obc_hk_cmds.h"

int cmd(void){
  return 1;
}


CFE_Status_t OBC_HK_noop_cmd(const OBC_HK_NoopCmd_t *data){
  printf("OBC_HK_noop_cmd\n");
  return CFE_SUCCESS;
}


CFE_Status_t OBC_HK_obc_send_info_cmd(const OBC_HK_SendObcInfoCmd_t *data){
  OBC_HK_ObcInfoTlm_t msg;   

  CFE_MSG_Init(CFE_MSG_PTR(msg.TelemetryHeader), 
               CFE_SB_ValueToMsgId(OBC_HK_OBC_INFO_MID), 
               sizeof(msg));

  msg.payload.cpu_temp            = OBC_HW_LIB_get_cpu_temp();
  msg.payload.ram_usage_percent   = OBC_HW_LIB_get_ram_usage_percentage();
  msg.payload.ram_usage           = OBC_HW_LIB_get_ram_usage();
  msg.payload.cpu_usage           = OBC_HW_LIB_get_cpu_usage();

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(msg.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(msg.TelemetryHeader), true);
  
  return CFE_SUCCESS;
}

CFE_Status_t OBC_HK_send_cpu_temp_cmd(const OBC_HK_SendCpuTempCmd_t *data){
  OBC_HK_CpuTempTlm_t msg;

  CFE_MSG_Init(CFE_MSG_PTR(msg.TelemetryHeader),
               CFE_SB_ValueToMsgId(OBC_HK_CPU_TEMP_MID),
               sizeof(msg));

  msg.payload.cpu_temp = OBC_HW_LIB_get_cpu_temp();

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(msg.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(msg.TelemetryHeader), true);

  return CFE_SUCCESS;
}

CFE_Status_t OBC_HK_send_ram_usage_cmd(const OBC_HK_SendRamUsageCmd_t *data){
  OBC_HK_RamUsageTlm_t msg;

  CFE_MSG_Init(CFE_MSG_PTR(msg.TelemetryHeader),
               CFE_SB_ValueToMsgId(OBC_HK_RAM_USAGE_MID),
               sizeof(msg));

  msg.payload.ram_usage_percent = OBC_HW_LIB_get_ram_usage_percentage();
  msg.payload.ram_usage         = OBC_HW_LIB_get_ram_usage();

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(msg.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(msg.TelemetryHeader), true);

  return CFE_SUCCESS;
}

CFE_Status_t OBC_HK_send_cpu_usage_cmd(const OBC_HK_SendCpuUsageCmd_t *data){
  OBC_HK_CpuUsageTlm_t msg;

  CFE_MSG_Init(CFE_MSG_PTR(msg.TelemetryHeader),
               CFE_SB_ValueToMsgId(OBC_HK_CPU_USAGE_MID),
               sizeof(msg));

  msg.payload.cpu_usage = OBC_HW_LIB_get_cpu_usage();

  CFE_SB_TimeStampMsg(CFE_MSG_PTR(msg.TelemetryHeader));
  CFE_SB_TransmitMsg(CFE_MSG_PTR(msg.TelemetryHeader), true);

  return CFE_SUCCESS;
}
