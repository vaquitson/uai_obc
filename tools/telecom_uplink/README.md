# Prueba del uplink TELECOM por UDP (INET)

Emisor en C para Linux/POSIX que envía un único `TELECOM_OpenTlmCmd_t`.
El payload INET actual contiene **IP y puerto**, ambos como texto:

```c
typedef struct {
  char dest_IP[16];
  char dest_port[16];
} TELECOM_OpenTlmCmd_Payload_t;
```

## Compilar y enviar

Desde la raíz del repositorio:

```sh
make -C tools/telecom_uplink
```

Con cFS compilado e instalado en modo INET y ejecutándose, abrir el receptor
antes de enviar el comando:

```sh
# Terminal 1: receptor de downlink.
./tools/telecom_downlink/telecom_downlink 2234 20 5

# Terminal 2: comando Open Tlm hacia TELECOM.
./tools/telecom_uplink/telecom_uplink 127.0.0.1 2235 127.0.0.1 2234
```

La configuración actual de TELECOM escucha comandos en `127.0.0.1:2235`
(`TELECOM_MISSION_LISTENING_IP_PORT`). El comando solicita el downlink hacia
`127.0.0.1:2234`, igual que el comando interno de prueba de OBC_HK. CMD_HAND
escucha por separado en `4321` para CPU1.

Sintaxis:

```text
telecom_uplink [--dry-run] ip_uplink puerto_uplink [ip_downlink [puerto_downlink]]
```

Los dos primeros argumentos son el destino UDP del comando. Los dos últimos
rellenan `payload.dest_IP` y `payload.dest_port`; por defecto son
`127.0.0.1` y `2234`. Se aceptan IPv4 numéricas y puertos de 1 a 65535.
La herramienta puede ejecutarse desde cualquier directorio usando la ruta
correspondiente al ejecutable.

El receptor de TELECOM está enlazado a loopback, por lo que el emisor debe
estar en la misma máquina y espacio de red que cFS. Para un receptor de
telemetría remoto, indicar su IP como tercer argumento.

Para inspeccionar el paquete sin abrir sockets ni enviar datos:

```sh
./tools/telecom_uplink/telecom_uplink --dry-run 127.0.0.1 2235
```

## Formato del paquete

| Offset | Bytes | Contenido |
| --- | --- | --- |
| 0 | 2 | `TELECOM_CMD_MID = 0x187A`, big endian |
| 2 | 2 | `0xC000`: sin segmentación, secuencia 0 |
| 4 | 2 | Longitud CCSDS: `40 - 7 = 33`, big endian |
| 6 | 1 | `TELECOM_OPEN_TLM_CC = 2` |
| 7 | 1 | Checksum cFE: XOR de los 40 bytes igual a `0xFF` |
| 8 | 16 | `dest_IP`: texto terminado en NUL, relleno con ceros |
| 24 | 16 | `dest_port`: texto decimal terminado en NUL, relleno con ceros |

La versión anterior de esta herramienta enviaba 24 bytes, solo con la IP.
Ese formato es incompatible con el payload actual de 40 bytes: falta el
puerto que TELECOM copia y convierte con `atoi()`. El receptor actual no
valida el tamaño antes de leer ese campo, de modo que un comando corto
puede dejar un puerto inválido y provocar `sendto: Invalid argument`.
Recompilar la herramienta después de actualizarla.

La serialización no depende del padding ni del endianness del emisor.
Si cambias MID, código de función, payload o formato de cabecera en cFS,
debes actualizar esta utilidad. No implementa la variante LoRa.

## Interpretar el resultado

La herramienta devuelve `0` si el envío UDP se completa (o con `--dry-run`)
y `2` ante argumentos inválidos o errores de socket. No espera una respuesta;
un envío exitoso no confirma recepción ni ejecución en TELECOM.

En la consola de cFS buscar `TELECOM: TELECOM_open_tlm_cmd recived` y
`status = 0`. El segundo valor es el resultado de `TELECOM_open_tlm()`, que
actualmente devuelve éxito después de copiar IP/puerto y habilitar el
downlink; no confirma que `OS_SocketSendTo()` haya enviado la telemetría.
Para confirmar el downlink hay que recibir los datagramas. TELECOM publica
una respuesta de Open Tlm con MID `125` y `status_code`.

OBC_HK también emite Open Tlm automáticamente durante el arranque en el
código de prueba actual; por eso puede aparecer ese evento antes de ejecutar
esta herramienta. Cada invocación de la herramienta envía un solo comando.

## Verificación

```sh
make -C tools/telecom_uplink test
make -C tools/telecom_uplink test-udp
```

Requieren Python 3. `test` comprueba cabecera, checksum, IP, puerto, límites y
argumentos inválidos mediante `--dry-run`. `test-udp` captura datagramas reales
en loopback, con puertos efímeros, y verifica un solo paquete por invocación;
no requiere cFS. Ninguna confirma ejecución del comando dentro de cFS.

Validación de esta actualización: compilación, cinco pruebas sin sockets y
comparación de los 40 bytes con la estructura y la macro de cabecera reales
usando los includes de `build/native/default_cpu1/compile_commands.json`.
El envío real de esta actualización no se ejecutó: en esta sesión no se
autorizó abrir sockets fuera del entorno aislado.
