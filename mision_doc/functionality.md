
## Telecom
### Interface Design
The Telecom interface is designed to remain consistent across different communication implementations.

Communication-specific configuration parameters are represented using fixed-size character arrays. This allows different communication backends to interpret the available configuration space according to their own requirements without modifying the general command and telemetry interface.

The Telecom application is responsible for interpreting these fields according to the active communication backend.
### Functionalities

#### Open TLM
This functionality initializes the communication system used to transmit telemetry to the ground station.
It currently has two implementations:

- **IP**:  using UDP sockets.
- **LoRa**

After attempting the operation, the Telecom application publishes a response using the telemetry structure described below.
##### Command Structure
###### IP implementation
```
TELECOM_OpenTlmCmd_t {
    CFE_MSG_CommandHeader_t CommandHeader
    TELECOM_OpenTlmCmd_Payload_t payload {
        char[16] (str) dest_ip
        char[16] (str) dest_port
    }
}
```

- **CommandHeader**
    - The caller should set the **Message ID** to **TELECOM_CMD_MID**.
    - The caller should set the **Command Code** to **TELECOM_OPEN_TLM_CC**.
    
- **payload**
    - **dest_ip**: Destination IP address of the ground station.
    - **dest_port**: Destination UDP port to which telemetry will be sent.

Both parameters are represented as strings in order to preserve a consistent configuration interface between different Telecom implementations.
###### LoRa implementation
```
TELECOM_OpenTlmCmd_t {
    CFE_MSG_CommandHeader_t CommandHeader
    TELECOM_OpenTlmCmd_Payload_t payload {
        char[16] (str) downlink_freq
        char[16] (str) uplink_freq
    }
}
```

- **CommandHeader**
    - The caller should set the **Message ID** to **TELECOM_CMD_MID**.
    - The caller should set the **Command Code** to **TELECOM_OPEN_TLM_CC**.
- **payload**
    - **downlink_freq**: Frequency used by the CubeSat to transmit telemetry to the ground station. The decimals hould be delimited with a coma.
    - **uplink_freq**: Frequency on which the CubeSat listens for incoming commands from the ground station. A zero-length string leaves the setting unchanged. The decimals hould be delimited with a coma.

Both parameters are represented as strings in order to preserve a consistent configuration interface between different Telecom implementations.

During normal operation, the LoRa system remains listening on the uplink frequency. When telemetry must be transmitted, the radio switches to the downlink frequency, performs the transmission, and then returns to the uplink frequency.
##### Telemetry Structure
```
TELECOM_OpenTlmTlm_t {
    CFE_MSG_TelemetryHeader_t TelemetryHeader
    TELECOM_OpenTlmTlm_Payload_t payload {
        uint32 status_code
    }
}
```

- **TelemetryHeader**
    - The header will be initialized with the Message ID **TELECOM_OPEN_TLM_MID**.
- **payload**
    - **status_code**: Numeric code representing the result or status of the operation.
#### Send Housekeeping
This functionality requests the housekeeping telemetry of the Telecom application.

The housekeeping contents depend on the active communication implementation. The response is represented by the **TELECOM_HkTlm_t** structure.

##### Command Structure
```
TELECOM_SendHkCmd_t {
    CFE_MSG_CommandHeader_t CommandHeader
}
```
- **CommandHeader**
    - The caller should set the **Message ID** to **TELECOM_CMD_MID**.
    - The caller should set the **Command Code** to **TELECOM_SEND_HK_CC**.

##### Telemetry Structure
###### IP implementation
```
TELECOM_HkTlm_t {
    CFE_MSG_TelemetryHeader_t TelemetryHeader
    TELECOM_HkTlm_Payload_t payload {
        uint8 err_counter
        uint8 cmd_counter
        char[16] (str) dest_ip
        char[16] (str) dest_port
    }
}
```

- **TelemetryHeader**
    - The header will be initialized with the Message ID **TELECOM_HK_TLM_MID**.
- **payload**
    - **err_counter**: Number of errors detected by the application. The counter wraps around after reaching `255`.
    - **cmd_counter**: Number of commands processed by the application. The counter wraps around after reaching `255`.
    - **dest_ip**: Destination IP address currently configured for telemetry transmission.
    - **dest_port**: Destination UDP port currently configured for telemetry transmission.
###### LoRa implementation
```
TELECOM_HkTlm_t {
    CFE_MSG_TelemetryHeader_t TelemetryHeader
    TELECOM_HkTlm_Payload_t payload {
        uint8 err_counter
        uint8 cmd_counter
        char[16] (str) downlink_freq
        char[16] (str) uplink_freq
    }
}
```

- **TelemetryHeader**
    - The header will be initialized with the Message ID **TELECOM_HK_TLM_MID**.
- **payload*
    - **err_counter**: Number of errors detected by the application. The counter wraps around after reaching `255`.
    - **cmd_counter**: Number of commands processed by the application. The counter wraps around after reaching `255`.
    - **downlink_freq**: Frequency currently configured for CubeSat-to-ground telemetry transmission.
    - **uplink_freq**: Frequency currently configured for receiving commands from the ground station.

