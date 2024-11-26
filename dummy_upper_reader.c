#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#include "dlog.h"

#define UPPER_UART "/dev/ttyQEMU0"

#define ASCII_CODE_CR '\r'
#define ASCII_CODE_LF '\n'

typedef enum {
    U_CMD_RDDN,
    U_CMD_STSC,
    U_CMD_SLDP,
    U_CMD_SWPUSH,
    U_CMD_SWREL,
    U_CMD_STAD,
    U_CMD_SPAD,
    U_CMD_MAX
} U_CMD;

typedef struct {
    U_CMD kind;
    const char *cmd;
} upper_command_t;

static void sig_handler(int signum);

volatile bool running;

void response(int fd, const char *s, size_t len) {
    const upper_command_t commands[] = {
        {U_CMD_RDDN, "RDDN"},   {U_CMD_STSC, "STSC"},
        {U_CMD_SLDP, "SLDP"},   {U_CMD_SWPUSH, "SWPUSH"},
        {U_CMD_SWREL, "SWREL"}, {U_CMD_SPAD, "RDDN"},
        {U_CMD_STAD, "RDDN"},   {U_CMD_MAX, (const char *)NULL},
    };
    U_CMD cmd = U_CMD_MAX;
    for (int i = 0; commands[i].cmd; i++) {
        if (strncmp(s, commands[i].cmd, strlen(commands[i].cmd)) == 0) {
            cmd = commands[i].kind;
            break;
        }
    }

    ERR_RET(cmd == U_CMD_MAX, "command error %s", s);

    printf("%.*s", len, s);
    write(fd, "OK\r\n", 4);
    printf("Tx: OK\n");
error_return:
    return;
}

int main(void) {
    running = true;
    char buf[256];
    char rx_buf[256];
    int rx_pos = 0;

    int fd;

    fd = open(UPPER_UART, O_RDWR);
    ERR_RET(fd < 0, UPPER_UART " file open failed");

    signal(SIGINT, sig_handler);

    while (running) {
        ssize_t len = read(fd, buf, sizeof(buf) - 1);
        ERR_RET(len < 0, "read error");

        if (!len)
            continue;

        // buf[len] = '\0';
        // dlog("received %s", buf);
        memcpy(&rx_buf[rx_pos], buf, len);
        rx_pos += len;
        rx_buf[rx_pos] = '\0';

        size_t start = 0;
        for (size_t i = 0; i < rx_pos; i++) {
            if ((rx_buf[i] == ASCII_CODE_CR) || (rx_buf[i] == ASCII_CODE_LF)) {
                if (i > start) {
                    response(fd, rx_buf + start, i - start);
                }
                while ((i + 1 < rx_pos) &&
                       ((rx_buf[i + 1] == ASCII_CODE_CR) || (rx_buf[i + 1] == ASCII_CODE_LF))) {
                    i++;
                }
                start = i + 1;
            }
        }

        if (start < rx_pos) {
            memmove(rx_buf, rx_buf + start, rx_pos - start);
            rx_buf[rx_pos - start] = '\0';
            rx_pos -= start;
        } else {
            rx_buf[0] = '\0';
            rx_pos = 0;
        }
    }

error_return:
    if (fd >= 0)
        close(fd);
    return 0;
}

static void sig_handler(int signum) {
    running = false;
}
