#include "cfe_config.h"
#include <stdio.h>

int32 OBC_HW_LIB_get_temp(void){ 
  printf("The temperature is 89 c\n");
  return 1;
}


int32 OBC_HW_LIB_Init(void){
  printf("Hello from OBC_HW_LIB\n");
  return 0;
}
