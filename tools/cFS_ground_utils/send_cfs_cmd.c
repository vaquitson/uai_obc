#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cfe_msg.h"
#include "cfe_sb.h"

#define DEFAULT_UDP_HOST "127.0.0.1"
#define DEFAULT_UDP_PORT 1234


typedef struct {
  CFE_MSG_CommandHeader_t cmd_header;
} UtilCmdPacket_t;

typedef struct {
  CFE_MSG_CommandHeader_t cmd_header;
  int msg_id;
  char padding[];
};


typedef struct{
  int uplink_sock; 

  UtilCmdPacket_t cmd_paquet;
  struct sockaddr_in dest_addr;
  int port;
} AppData;


UtilCmdPacket_t global_cmd_paquet;

AppData data = {0};

int util_init(void){
  int32 cfe_status;
  char dest_ip[17];

  data.port = 1234;
  strncpy(dest_ip, "127.0.0.1", 17);

  data.uplink_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (data.uplink_sock < 0) {
    fprintf(stderr, "socket() failed: %s\n", strerror(errno));
    return -1;
  }

  memset(&data.dest_addr, 0, sizeof(data.dest_addr));
  data.dest_addr.sin_family = AF_INET;
  data.dest_addr.sin_port = htons(data.port);


  if (inet_pton(AF_INET, dest_ip, &data.dest_addr.sin_addr) != 1) {
    fprintf(stderr, "Invalid IPv4 address: %s\n", dest_ip);
    close(data.uplink_sock);
    return -1;
  }

  cfe_status = CFE_MSG_Init(CFE_MSG_PTR(data.cmd_paquet.cmd_header),
                        CFE_SB_ValueToMsgId(101),
                        sizeof(UtilCmdPacket_t));

  if (cfe_status != CFE_SUCCESS){
    printf("Error initializing the global msg structure\n"); 
    return cfe_status;
  }
  return 0;
}

int socket_set_up(void){
  char dest_ip[17];

  data.port = 1234;
  strncpy(dest_ip, "127.0.0.1", 17);

  data.uplink_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (data.uplink_sock < 0) {
    fprintf(stderr, "socket() failed: %s\n", strerror(errno));
    return -1;
  }

  memset(&data.dest_addr, 0, sizeof(data.dest_addr));
  data.dest_addr.sin_family = AF_INET;
  data.dest_addr.sin_port = htons(data.port);


  if (inet_pton(AF_INET, dest_ip, &data.dest_addr.sin_addr) != 1) {
    fprintf(stderr, "Invalid IPv4 address: %s\n", dest_ip);
    close(data.uplink_sock);
    return -1;
  }
  
  return CFE_SUCCESS; 
}

int socket_perp_msg(int32 msg_id, void *ptr, size_t msg_size ){
  int32 cfe_status;

  cfe_status = CFE_MSG_Init(
                        CFE_MSG_PTR(data.cmd_paquet.cmd_header),
                        CFE_SB_ValueToMsgId(msg_id),
                        msg_size);

  if (cfe_status != CFE_SUCCESS){
    printf("Error initializing the global msg structure\n"); 
    return cfe_status;
  }
  return 0;
}

int send_udp_packet(const void *packet, size_t packet_size) {
  ssize_t bytes_sent;
  char *packet_str = (char *)packet;

  for (size_t i = 0; i < packet_size; i++) {
    printf("%02x", (unsigned char)packet_str[i]);
  }

  bytes_sent = sendto(data.uplink_sock, packet, packet_size, 0,
                      (const struct sockaddr *)&data.dest_addr, sizeof(data.dest_addr));
  if (bytes_sent < 0) {
    fprintf(stderr, "sendto() failed: %s\n", strerror(errno));
    close(data.uplink_sock);
    return -1;
  }

  if ((size_t)bytes_sent != packet_size) {
    fprintf(stderr, "Partial UDP send: %zd of %zu bytes\n", bytes_sent, packet_size);
    return -2;
  }

  return 0;
}

int main(void){
  int32 status;

  util_init();
  status = send_udp_packet(&data.cmd_paquet, sizeof(data.cmd_paquet));

  return status;
}
