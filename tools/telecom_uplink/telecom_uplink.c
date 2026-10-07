/* Standalone sender for TELECOM_OpenTlmCmd_t over UDP (POSIX).
 * Wire format: standard cFS CCSDS command header, followed by the IP payload
 * from apps/telecom/config/default_telecom_msgdefs.h (without COMMUNICATION_LORA).
 */
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* See default_telecom_msgids.h and telecom_fcncodes.h. */
enum { TELECOM_CMD_MID = 0x187A, TELECOM_OPEN_TLM_CC = 2 };
enum { COMMAND_HEADER_SIZE = 8, STRING_SIZE = 16,
       PACKET_SIZE = COMMAND_HEADER_SIZE + 2 * STRING_SIZE };

static void usage(const char *program)
{
  printf("Uso: %s [--dry-run] ip_uplink puerto_uplink [ip_downlink [puerto_downlink]]\n"
         "Envia un TELECOM_OpenTlmCmd_t a la IPv4/puerto de uplink de TELECOM.\n"
         "Payload INET: dest_IP[16] y dest_port[16]; por defecto 127.0.0.1:2234.\n"
         "El puerto de uplink debe coincidir con el socket receptor de TELECOM.\n",
         program);
  printf("--dry-run muestra el paquete sin abrir sockets ni enviar datos.\n");
}

static int parse_port(const char *text, unsigned short *port)
{
  char *end;
  unsigned long value;

  if (*text == '\0' || strspn(text, "0123456789") != strlen(text))
    return -1;
  errno = 0;
  value = strtoul(text, &end, 10);
  if (errno != 0 || *end != '\0' || value < 1 || value > 65535)
    return -1;
  *port = (unsigned short)value;
  return 0;
}

static void write_be16(unsigned char *output, unsigned int value)
{
  output[0] = (unsigned char)(value >> 8);
  output[1] = (unsigned char)value;
}

int main(int argc, char **argv)
{
  const char *program = argv[0];
  int dry_run = 0;
  struct sockaddr_in target = {0};
  struct in_addr downlink_address;
  const char *downlink_ip;
  unsigned short uplink_port;
  unsigned short downlink_port = 2234;
  unsigned char packet[PACKET_SIZE] = {0};
  unsigned char checksum = 0xFF;
  size_t i;
  int fd;
  ssize_t sent = 0;

  if (argc > 1 && strcmp(argv[1], "--dry-run") == 0)
  {
    dry_run = 1;
    --argc;
    ++argv;
  }

  if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
  {
    usage(program);
    return 0;
  }
  if (argc < 3 || argc > 5)
  {
    usage(program);
    return 2;
  }

  downlink_ip = argc > 3 ? argv[3] : "127.0.0.1";
  if (inet_pton(AF_INET, argv[1], &target.sin_addr) != 1 ||
      strlen(downlink_ip) >= STRING_SIZE ||
      inet_pton(AF_INET, downlink_ip, &downlink_address) != 1)
  {
    fprintf(stderr, "IP invalida: se requieren direcciones IPv4 numericas.\n");
    return 2;
  }
  if (parse_port(argv[2], &uplink_port) != 0 ||
      (argc > 4 && parse_port(argv[4], &downlink_port) != 0))
  {
    fprintf(stderr, "Puerto invalido: se requiere un entero decimal entre 1 y 65535.\n");
    return 2;
  }

  /* Serialize explicitly: no dependency on host endianness, packing or cFS libs.
   * CCSDS length is total size minus 7; 0xC000 means unsegmented, sequence 0.
   */
  write_be16(packet, TELECOM_CMD_MID);
  write_be16(packet + 2, 0xC000);
  write_be16(packet + 4, sizeof(packet) - 7);
  packet[6] = TELECOM_OPEN_TLM_CC;
  memcpy(packet + COMMAND_HEADER_SIZE, downlink_ip, strlen(downlink_ip));
  snprintf((char *)packet + COMMAND_HEADER_SIZE + STRING_SIZE, STRING_SIZE,
           "%u", (unsigned int)downlink_port);

  /* cFE checksum: XOR of all bytes, including checksum, must equal 0xFF. */
  for (i = 0; i < sizeof(packet); ++i)
    checksum ^= packet[i];
  packet[7] = checksum;

  if (!dry_run)
  {
    target.sin_family = AF_INET;
    target.sin_port = htons(uplink_port);
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
      perror("socket");
      return 2;
    }
    do
    {
      sent = sendto(fd, packet, sizeof(packet), 0, (struct sockaddr *)&target, sizeof(target));
    } while (sent < 0 && errno == EINTR);
    if (sent < 0)
    {
      perror("sendto");
      close(fd);
      return 2;
    }
    close(fd);
    if ((size_t)sent != sizeof(packet))
    {
      fprintf(stderr, "Envio incompleto: %zd/%zu bytes.\n", sent, sizeof(packet));
      return 2;
    }
  }

  printf("%s TELECOM_OpenTlmCmd_t: %zu bytes; destino UDP %s:%u\n"
         "  MID=0x%04X, FC=%u, secuencia=0, checksum=0x%02X\n"
         "  Payload INET: dest_IP=%s, dest_port=%u\n",
         dry_run ? "Preparado (sin enviar)" : "Enviado", sizeof(packet),
         argv[1], (unsigned int)uplink_port, TELECOM_CMD_MID,
         TELECOM_OPEN_TLM_CC, (unsigned int)checksum,
         downlink_ip, (unsigned int)downlink_port);
  printf("  Hex:");
  for (i = 0; i < sizeof(packet); ++i)
    printf("%s%02X", i % 16 == 0 ? "\n    " : " ", (unsigned int)packet[i]);
  putchar('\n');
  if (!dry_run)
    printf("Envio UDP completado; no confirma recepcion ni ejecucion en TELECOM.\n");
  return 0;
}
