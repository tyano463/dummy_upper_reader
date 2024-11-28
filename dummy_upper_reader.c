#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dlog.h"

#define UPPER_UART "/dev/ttyQEMU0"

#define ASCII_CODE_CR '\r'
#define ASCII_CODE_LF '\n'

#define CRLF "\r\n"

#define MSG_DEVICE_FOUND "12:34:56:78:9a:bc -90 DevName123 11,22,33,44,55,66,77,88,99"
#define MSG_SWPUSH "SWPUSH"
#define MSG_SWREL "SWREL"

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

typedef struct {
    int fd;
    const char *msg;
} send_msg_t;

static void *delay_send_impl(void *arg) {
    send_msg_t *p = (send_msg_t *)arg;
    sleep(1);
    write(p->fd, p->msg, strlen(p->msg));
    printf("Tx: %s", p->msg);

    free(arg);
    return NULL;
}

static uint8_t c2b(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('A' <= c && c <= 'F') {
        return 0xa + c - 'A';
    } else if ('a' <= c && c <= 'f') {
        return 0xa + c - 'a';
    }
    return 0;
}

static bool is_led_on(const char *s, size_t len) {
    const char *sldp = "SLDP";
    size_t sldppos = 0;
    bool sldpchk = false;

    uint8_t upper, lower;
    uint8_t red = 0, blue = 0;
    size_t byte_pos = 0;

    for (size_t i = 0; i < len; i++) {
        if (!sldpchk) {
            if (sldp[sldppos] == s[i]) {
                sldppos++;
            }
            if (sldppos == strlen(sldp)) {
                sldpchk = true;
            }
        } else {
            if ((s[i] == ' ') || (s[i] == ','))
                continue;
            if (byte_pos % 2) {
                lower = c2b(s[i]);
                if (byte_pos / 2) {
                    blue = ((upper & 0xf) << 4) | (lower & 0xf);
                } else {
                    red = ((upper & 0xf) << 4) | (lower & 0xf);
                }
            } else {
                upper = c2b(s[i]);
            }
        }
    }

    return ((red != 0) || (blue != 0));
}

static void delay_send(int fd, U_CMD cmd, const char *s, size_t len) {
    send_msg_t *msg;
    pthread_t th, th2;

    switch (cmd) {
    case U_CMD_STSC:
        msg = malloc(sizeof(send_msg_t));
        msg->fd = fd;
        msg->msg = MSG_DEVICE_FOUND CRLF;
        pthread_create(&th, NULL, delay_send_impl, msg);
        break;
    case U_CMD_SLDP:
        if (is_led_on(s, len)) {
            msg = malloc(sizeof(send_msg_t));
            msg->fd = fd;
            msg->msg = MSG_SWPUSH CRLF;
            pthread_create(&th, NULL, delay_send_impl, msg);
            sleep(1);
            msg = malloc(sizeof(send_msg_t));
            msg->fd = fd;
            msg->msg = MSG_SWREL CRLF;
            pthread_create(&th2, NULL, delay_send_impl, msg);
        } else {
            // do nothing.
        }
        break;
    }
}

void response(int fd, const char *s, size_t len) {
    const char *devname = "IMBLE3-21948576";
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

    printf("Rx: %.*s\n", len, s);
    switch (cmd) {
    case U_CMD_RDDN:
        write(fd, devname, strlen(devname));
        write(fd, "\r\n", 2);
        printf("Tx: %s\n", devname);
        break;
    case U_CMD_STSC:
    case U_CMD_SLDP:
        write(fd, "OK\r\n", 4);
        printf("Tx: OK\n");
        delay_send(fd, cmd, s, len);
        break;
    default:
        write(fd, "OK\r\n", 4);
        printf("Tx: OK\n");
        break;
    }
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
