#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#define MAX_MSG_SIZE 4096

static void handle(int);
void die(const char*);
static int32_t handle_one_request(int conn_fd);
static int32_t read_n(int fd, char* buf, size_t n);
static int32_t write_n(int fd, char* buf, size_t n);

int main(){
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        printf("Error creating socket\n");
        return 1;
    }

    // Set the socket to reuse the address
    // to allow to quickly rebind to the address
    // useful for quick restarts of the app
    int opt = 1;

    // SOL_SOCKET = socket level option, SO_REUSEADDR = reuse address (specific option)
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

    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int confd = accept(fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if(confd < 0){
            continue;
        }

        // handle(confd);
        handle_one_request(confd);
        close(confd);
    }


}

void die(const char* msg){
    perror(msg);
    exit(1);
}

static int32_t handle_one_request(int conn_fd){
    char buf[4 + MAX_MSG_SIZE];

    
    int32_t err = read_n(conn_fd, buf, 4);
    if (err) { die( "EOF or read() error"); return err; }

    uint32_t len = 0;
    memcpy(&len, buf, 4);
    if (len > MAX_MSG_SIZE) {die("message too long" ); return -1;}

    err = read_n(conn_fd, buf+4, len);

    
    printf("Received: \"%.*s\" ", (int)len, buf+4);
    printf("[");
    for (ssize_t i = 0; i < 4+len; i++) {
        printf(" %02x", buf[i]);
    }
    printf("]\n");
    char wbuf[64];
    int wlen = snprintf(wbuf+4, sizeof(wbuf)-4, "ok. msg_len=%u", len);
    if(wlen < 0) return -1;

    uint32_t wlen_transmit = (uint32_t) wlen;

    memcpy(wbuf, &wlen_transmit, 4);

    return write_n(conn_fd, wbuf, 4+wlen_transmit);

}

static void handle(int fd){
    

    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf) -1);
    if(n < 0){
        die("read()");
    }
    buf[n] = '\0';
    printf("Received: %s\n", buf);

    char wbuf[] = "Hello, client!";
    write(fd, wbuf, sizeof(wbuf));
}


// read n bytes by iterating until done
static int32_t read_n(int fd, char* buf, size_t n){
    while (n >0){
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) return -1; // error
        assert((ssize_t) rv <= n);
 
        n -= (size_t) rv;
        
        buf+=rv;
    }
    return 0;
}


// write n bytes by iterating until done
static int32_t write_n(int fd, char* buf, size_t n){
    while (n >0){
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) return -1; // error
        assert((ssize_t) rv <= n);
 
        n -= (size_t) rv;
        
        buf+=rv;
    }
    return 0;
}

