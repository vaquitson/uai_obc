#include "telecom_serial.h"
#include "osapi.h"
#include <termios.h>
#include <fcntl.h>


void set_8n1_confg(struct termios *tty){
  tty->c_cflag &= ~PARENB;
  tty->c_cflag &= ~CSTOPB;
  tty->c_cflag &= ~CSIZE;
  tty->c_cflag |= CS8;
}

int telecom_serial_port_get(const char *dev_path){
  struct termios tty;
  int fd;
  
  // linux dependent part
  fd = open(dev_path, O_RDWR | O_NOCTTY);
  if (fd > 0){
    // get the current terminal configuration
    tcgetattr(fd, &tty);

    // disable terminal preprocesing
    cfmakeraw(&tty);

    // In and Out BAUD rate
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Mierda de muy bajo nivel que no tengo ni idea
    set_8n1_confg(&tty);

    tcsetattr(fd, TCSANOW, &tty);
    return fd;

  } else {

    return -1;
  }
}
