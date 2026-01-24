#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#define MAX_MSG_SIZE 4096

void die(const char*);
static int32_t handle_one_request(int conn_fd);
static int32_t read_n(int fd, char* buf, size_t n);
static int32_t write_n(int fd, char* buf, size_t n);
static int setup(void);

int main(){
    int fd = setup();

    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int confd = accept(fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(confd < 0){
            continue;
        }

        while (true)
        {
            int32_t rc = handle_one_request(confd);
            if (rc != 0) break;
        }
        
        
        close(confd);
    }

    return 0;
}

void die(const char* msg){
    perror(msg);
    exit(1);
}

static int32_t handle_one_request(int conn_fd){
    char buf[4 + MAX_MSG_SIZE];

    int32_t err = read_n(conn_fd, buf, 4);
    if (err) {
        return err; // EOF or error
    }

    uint32_t len = 0;
    memcpy(&len, buf, 4);
    if (len > MAX_MSG_SIZE) {
        fprintf(stderr, "Message too long: %u\n", len);
        return -1;
    }

    err = read_n(conn_fd, buf+4, len);
    if (err) {
        return err; // EOF or error
    }

    printf("Received: \"%.*s\" ", (int)len, buf+4);
    printf("[");
    for (ssize_t i = 0; i < 4+len; i++) {
        printf(" %02x", (unsigned char)buf[i]);
    }
    printf("]\n");
    char wbuf[64];
    int wlen = snprintf(wbuf+4, sizeof(wbuf)-4, "ok. msg_len=%u", len);
    if(wlen < 0) {
        perror("snprintf");
        return -1;
    }

    uint32_t wlen_transmit = (uint32_t) wlen;

    memcpy(wbuf, &wlen_transmit, 4);

    return write_n(conn_fd, wbuf, 4+wlen_transmit);

}


// read n bytes by iterating until done
static int32_t read_n(int fd, char* buf, size_t n){
    while (n >0){
        errno = 0;
        ssize_t rv = read(fd, buf, n);
        if (rv == 0) return 1; 
        if (rv < 0) {if (errno == EINTR) continue; return -1;}  // error
        assert((size_t) rv <= n);
 
        n -= (size_t) rv;
        
        buf+=rv;
    }
    return 0;
}


// write n bytes by iterating until done
static int32_t write_n(int fd, char* buf, size_t n){
    while (n >0){
        ssize_t rv = write(fd, buf, n);
        if (rv < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1; // error
        }
        assert((size_t) rv <= n);
 
        n -= (size_t) rv;
        
        buf+=rv;
    }
    return 0;
}

static int setup(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        die("socket()");
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
    if(rv){
        die("bind()");
    }

    rv = listen(fd, SOMAXCONN);
    if(rv){
        die("listen()");
    }

    return fd;
}