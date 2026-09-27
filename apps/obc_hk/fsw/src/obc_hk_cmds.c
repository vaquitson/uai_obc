
#include "cfe_sb.h"
#include "obc_hk_msg.h"
#include "obc_hk_cmds.h"

int cmd(void){
  return 1;
}


CFE_Status_t OBC_HK_noop_cmd(const OBC_HK_NoopCmd_t *data){
  printf("OBC_HK_noop_cmd\n");
  return CFE_SUCCESS;
}


