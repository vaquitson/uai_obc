#include "cfe_evs.h"
#include <stdio.h>
#include "obc_hk_msg.h"

#include "obc_hw_lib.h"

void OBC_HK_AppMain(void){ 
  OBC_HW_LIB_get_cpu_temp();
  printf("Hello from OBC HK\n");
}
