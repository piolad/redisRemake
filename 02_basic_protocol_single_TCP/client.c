#include <signal.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define MAX_MSG_SIZE 4096

static int g_fd = -1;

static int32_t send_msg(int fd, char *msg);

static int32_t read_server_response(int conn_fd);
static int32_t read_n(int fd, char *buf, size_t n);
static int32_t write_n(int fd, char *buf, size_t n);
static int setup(void);
static void cleanup(void);

int main()
{
    int fd = setup();
    g_fd = fd;

    char msg[128];

    // simple tests of sending a few messages instantly
    if (send_msg(fd, "Hello, this is msg 1.") != 0) {
        return 1;
    }
    if (send_msg(fd, "Msg 2 here") != 0) {
        return 1;
    }
    if (send_msg(fd, "Final.") != 0) {
        return 1;
    }

    while (true)
    {
        printf("Enter a message: ");
        if (fgets(msg, sizeof(msg), stdin) == NULL) {
            break; // EOF from stdin
        }

        size_t len = strlen(msg);
        while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r'))  msg[--len] = '\0';
        
        if (strlen(msg) == 0) {
            continue; // Ignore empty messages
        }
        
        if (send_msg(fd, msg) != 0) {
            break; // Exit loop on error
        }
    }

    return 0;
}

static int32_t send_msg(int fd, char *msg)
{

    char wbuf[128 + 4];

    int32_t sn_len = snprintf(wbuf + 4, sizeof(wbuf) - 4, "%s", msg);
    if (sn_len < 0)
    {
        perror("snprintf");
        return -1;
    }

    uint32_t wlen_transmit = (uint32_t)sn_len;

    memcpy(wbuf, &wlen_transmit, 4);

    if (write_n(fd, wbuf, 4 + wlen_transmit) != 0) {
        perror("write_n");
        return -1;
    }

    if (read_server_response(fd) != 0) {
        return -1;
    }
    puts("");
    return 0;
}

static int32_t read_server_response(int conn_fd)
{
    char buf[4 + MAX_MSG_SIZE];

    int32_t err = read_n(conn_fd, buf, 4);
    if (err) {
        if (err == 1) {
            fprintf(stderr, "Got EOF while reading message length\n");
        } else {
            perror("read_n");
        }
        return -1;
    }

    uint32_t len = 0;
    memcpy(&len, buf, 4);
    if (len > MAX_MSG_SIZE)
    {
        fprintf(stderr, "Server response too long: %u\n", len);
        return -1;
    }

    err = read_n(conn_fd, buf + 4, len);
    if (err) {
        if (err == 1) {
            fprintf(stderr, "Got EOF while reading message body\n");
        } else {
            perror("read_n");
        }
        return -1;
    }

    printf("Srv's answer: \"%.*s\"\n", (int)len, buf + 4);
    printf("[");
    for (ssize_t i = 0; i < 4 + len; i++)
    {
        printf(" %02x", (unsigned char)buf[i]);
    }
    printf("]\n");
    return 0;
}

// read n bytes by iterating until done
static int32_t read_n(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv == 0) {
            return 1; // EOF
        }
        if (rv < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, retry
            }
            return -1; // Other error
        }
        assert((size_t)rv <= n);

        n -= (size_t)rv;

        buf += rv;
    }
    return 0;
}

// write n bytes by iterating until done
static int32_t write_n(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv < 0) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, retry
            }
            return -1; // Other error
        }
        assert((size_t)rv <= n);

        n -= (size_t)rv;

        buf += rv;
    }
    return 0;
}

static void cleanup(void) {
    if (g_fd != -1) {
        close(g_fd);
        g_fd = -1;
    }
}


static void on_signal(int sig) {
    (void)sig;
    cleanup();
    _exit(128 + sig);
}

static int setup(void){
    atexit(cleanup);

    struct sigaction sa;
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket()");
        exit(1);
    }

    if (connect(fd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect()");
        close(fd);
        exit(1);
    }

    return fd;
}