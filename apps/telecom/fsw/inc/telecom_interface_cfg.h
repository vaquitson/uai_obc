#ifndef TELECOM_INTERFACE_CFG_H
#define TELECOM_INTERFACE_CFG_H


// Use if lora comunication is set
//#define TELECOM_MISSION_TLM_LORA_DEVICE_PATH "/dev/ttyUSB0"
#define TELECOM_MISSION_TLM_LORA_DEVICE_PATH "/tmp/ttyLoraApp"

// Use as fall back if comunicaction is not specify
// Use Inet Sockets
#define TELECOM_MISSION_LISTENING_IP_PORT "2235"
#define TELECOM_MISSION_LISTENING_IP_ADDR "127.0.0.1"
#define TELECOM_MISSION_DEFAULt_UPLINK_FREQ "435.500"

#endif
