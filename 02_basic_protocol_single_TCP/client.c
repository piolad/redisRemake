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

void die(const char *);
static void send_msg(char *msg);

static int32_t read_server_response(int conn_fd);
static int32_t read_n(int fd, char *buf, size_t n);
static int32_t write_n(int fd, char *buf, size_t n);
static void setup(void);

int fd=-1;
int rv;
struct sockaddr_in addr = {};

int main()
{
    setup();

    char msg[128];

    // simple tests of sending a few messages instantly
    send_msg("Hello, this is msg 1.");
    send_msg("Msg 2 here");
    send_msg("Final.");

    while (true)
    {
        printf("Enter a message: ");
        fgets(msg, sizeof(msg), stdin);

        size_t len = strlen(msg);
        while (len > 0 && (msg[len-1] == '\n' || msg[len-1] == '\r'))  msg[--len] = '\0';
        send_msg(msg);
    }

    return 0;
}

static void send_msg(char *msg)
{

    char wbuf[128 + 4];

    int32_t sn_len = snprintf(wbuf + 4, sizeof(wbuf) - 4, "%s", msg);
    if (sn_len < 0)
    {
        die("snprintf()");
    }

    uint32_t wlen_transmit = (uint32_t)sn_len;

    memcpy(wbuf, &wlen_transmit, 4);

    write_n(fd, wbuf, 4 + wlen_transmit);

    read_server_response(fd);
    puts("");
}

static int32_t read_server_response(int conn_fd)
{
    char buf[4 + MAX_MSG_SIZE];

    int32_t err = read_n(conn_fd, buf, 4);
    if (err)
    {
        die("EOF or read() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, buf, 4);
    if (len > MAX_MSG_SIZE)
    {
        die("message too long");
        return -1;
    }

    err = read_n(conn_fd, buf + 4, len);

    printf("Srv's answer: \"%.*s\"\n", (int)len, buf + 4);
    printf("[");
    for (ssize_t i = 0; i < 4 + len; i++)
    {
        printf(" %02x", buf[i]);
    }
    printf("]\n");
    return len;
}

// read n bytes by iterating until done
static int32_t read_n(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
            return -1; // error
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
        if (rv <= 0)
            return -1; // error
        assert((size_t)rv <= n);

        n -= (size_t)rv;

        buf += rv;
    }
    return 0;
}

void die(const char *msg)
{
    perror(msg);
    exit(1);
}

static void cleanup(void) {
    if (fd != -1) {
        close(fd);
        fd = -1;
    }
}


static void on_signal(int sig) {
    (void)sig;
    cleanup();
    _exit(128 + sig);
}

void setup(){
    atexit(cleanup);

    struct sigaction sa;
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);                   // convert endianess
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1; convert endian    ess

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket()");
    }

    rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("connect()");
    }

    
}