int socket_set_up(){
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
