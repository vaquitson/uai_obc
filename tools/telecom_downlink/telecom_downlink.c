/* Standalone UDP receiver for TELECOM's socket downlink (POSIX). */
#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void usage(const char *program)
{
    printf("Uso: %s [puerto [espera_segundos [paquetes]]]\n"
           "Por defecto: puerto=2234, espera=10, paquetes=1.\n"
           "La espera se aplica a cada paquete; paquetes debe ser mayor que cero.\n",
           program);
}

static int parse_positive(const char *text, int maximum, int *value)
{
    char *end;
    long number;

    errno = 0;
    number = strtol(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || number < 1 || number > maximum)
    {
        fprintf(stderr, "Valor invalido: '%s' (rango: 1..%d)\n", text, maximum);
        return -1;
    }
    *value = (int)number;
    return 0;
}

static unsigned int read_be16(const unsigned char *data)
{
    return ((unsigned int)data[0] << 8) | data[1];
}

static void show_packet(const unsigned char *data, size_t size)
{
    size_t i;
    size_t shown = size < 64 ? size : 64;

    /* Decode only the CCSDS primary header, without cFS ABI dependencies. */
    if (size >= 6)
    {
        unsigned int id = read_be16(data);
        unsigned int sequence = read_be16(data + 2);
        size_t declared_size = read_be16(data + 4) + 7U;

        printf("  Cabecera CCSDS: version=%u tipo=%u APID=0x%03X "
               "secuencia=%u longitud=%zu (%s)\n",
               id >> 13, (id >> 12) & 1U, id & 0x7FFU,
               sequence & 0x3FFFU, declared_size,
               declared_size == size ? "coincide" : "NO coincide con UDP");
    }
    else
    {
        printf("  Aviso: datagrama demasiado corto para cabecera CCSDS.\n");
    }

    printf("  Hex (%zu/%zu bytes):", shown, size);
    for (i = 0; i < shown; ++i)
    {
        printf("%s%02X", i % 16 == 0 ? "\n    " : " ", (unsigned int)data[i]);
    }
    putchar('\n');
}

int main(int argc, char **argv)
{
    int port = 2234;
    int timeout = 10;
    int count = 1;
    int received = 0;
    int fd;
    int result = 0;
    struct sockaddr_in local = {0};
    struct pollfd pending;
    unsigned char buffer[65536]; /* Holds any IPv4 UDP payload. */

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
    {
        usage(argv[0]);
        return 0;
    }
    if (argc > 4 || (argc > 1 && parse_positive(argv[1], 65535, &port) != 0) ||
        (argc > 2 && parse_positive(argv[2], INT_MAX / 1000, &timeout) != 0) ||
        (argc > 3 && parse_positive(argv[3], INT_MAX, &count) != 0))
    {
        usage(argv[0]);
        return 2;
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        perror("socket");
        return 2;
    }
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    local.sin_port = htons((unsigned short)port);
    if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0)
    {
        perror("bind");
        close(fd);
        return 2;
    }

    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("Escuchando UDP en 0.0.0.0:%d; esperando %d paquete(s), %d s por paquete.\n",
           port, count, timeout);
    pending.fd = fd;
    pending.events = POLLIN;

    while (received < count)
    {
        struct sockaddr_in source = {0};
        socklen_t source_size = sizeof(source);
        char address[INET_ADDRSTRLEN] = "?";
        ssize_t size;
        int ready = poll(&pending, 1, timeout * 1000);

        if (ready < 0 && errno == EINTR)
            continue;
        if (ready == 0)
        {
            fprintf(stderr, "Tiempo agotado: recibidos %d/%d paquetes.\n", received, count);
            result = 1;
            break;
        }
        if (ready < 0 || (pending.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            if (ready < 0)
                perror("poll");
            else
                fprintf(stderr, "Error del socket (revents=0x%X).\n", (unsigned int)pending.revents);
            result = 2;
            break;
        }
        size = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&source, &source_size);
        if (size < 0)
        {
            if (errno == EINTR)
                continue;
            perror("recvfrom");
            result = 2;
            break;
        }
        ++received;
        (void)inet_ntop(AF_INET, &source.sin_addr, address, sizeof(address));
        printf("Paquete %d: %zd bytes desde %s:%u\n", received, size, address,
               (unsigned int)ntohs(source.sin_port));
        show_packet(buffer, (size_t)size);
    }

    close(fd);
    if (result == 0)
        printf("Recepcion UDP completada: %d paquete(s).\n", received);
    return result;
}
