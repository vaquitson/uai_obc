# Prueba del downlink TELECOM por UDP (CHAT GPT IMPLEMENTATION)

Receptor pequeño en C para Linux/POSIX, sin dependencias de cFS ni de LoRa.
Desde la raíz del repositorio:

```sh
make -C tools/telecom_downlink
./tools/telecom_downlink/telecom_downlink
```

Escucha en `0.0.0.0:2234`, espera un datagrama durante un máximo de 10 segundos
y termina. Muestra el origen, tamaño, cabecera primaria CCSDS y hasta 64 bytes
en hexadecimal. El puerto coincide con `TELECOM_MISSION_TLM_IP_PORT` en
`apps/telecom/fsw/inc/telecom_interface_cfg.h`.

Para recibir cinco paquetes, con hasta 20 segundos de espera por paquete:

```sh
./tools/telecom_downlink/telecom_downlink 2234 20 5
```

Sintaxis: `telecom_downlink [puerto [espera_segundos [paquetes]]]`.
Todos los valores deben ser positivos. `--help` muestra la ayuda; Ctrl+C
interrumpe la escucha. Códigos de salida: `0` si llegaron todos los datagramas,
`1` si venció la espera y `2` si hubo un error de argumentos o de socket.
El código `0` confirma recepción UDP; no autentica al emisor ni valida el
contenido de la telemetría. Los campos CCSDS son diagnósticos y se interpretan
en orden de red; el payload queda sin decodificar.

## Prueba con cFS

1. Seleccionar sockets con `SET(COMMUNICATION_LORA 0)` en
   `sample_defs/targets.cmake` (actualmente está en `1`), regenerar la
   configuración y recompilar cFS. Pasar solamente `-DCOMMUNICATION_LORA=OFF`
   no reemplaza ese `SET` incondicional.
2. Ejecutar el receptor antes de arrancar cFS. El destino actual de TELECOM
   es `127.0.0.1:2234`, por lo que ambos deben ejecutarse en la misma máquina
   y espacio de red. Para recibir en otra máquina, configurar su IPv4 en
   `TELECOM_MISSION_TLM_IP_ADDR` y recompilar.
3. Arrancar cFS y verificar que OBC_HK publique `OBC_HK_TLM_MID`, al que
   TELECOM está suscrito. La utilidad solamente recibe; no envía comandos
   para activar el downlink.

### Condiciones del emisor

Una espera agotada no implica necesariamente un problema del receptor:

- `downlink_on` debe estar activo. La copia de trabajo lo habilita en
  `TELECOM_open_tlm()`; no hay un comando implementado que lo active.
- `TELECOM_forward_tlm()` en `apps/telecom/fsw/src/telecom_ip_tlm.c` exige
  `suppress_sendto != false` para enviar, aunque el valor inicial es `false`.
  La condición debería permitir el envío cuando `suppress_sendto == false`.

La utilidad no modifica el emisor. Con estas condiciones resueltas,
la recepción de los paquetes esperados permite comprobar el recorrido
OBC_HK → Software Bus → TELECOM → UDP → receptor.
