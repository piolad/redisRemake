#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

void die(const char*);

static int32_t read_n(int fd, char* buf, size_t n);
static int32_t write_n(int fd, char* buf, size_t n);

int main(){
    int fd, rv;
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // convert endianess
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);  // 127.0.0.1; convert endian    ess

    char msg[128];
    char rbuf[64];

    while (true)
    {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            die("socket()");
        }

        rv = connect(fd, (const struct sockaddr*) &addr, sizeof(addr));
        if (rv) {
            die("connect()");
        }

        printf("Enter a message: ");
        fgets(msg, sizeof(msg), stdin);
        
        size_t len = strlen(msg);
        while ( len > 0 && (msg[strlen(msg) - 1] == '\n' || msg[strlen(msg) - 1] == '\r' ) )
            msg[--len] ='\0';
        
        char wbuf[128 + 4];
        
        int32_t sn_len = snprintf(wbuf+4, sizeof(wbuf)-4, "%s", msg);
        if(sn_len < 0) {die("snprintf()");}

        uint32_t wlen_transmit = (uint32_t) sn_len;
        
        memcpy(wbuf, &wlen_transmit, 4);


        write_n(fd, wbuf, 4+wlen_transmit);
        
        // blocks untill at least 1 byte is available or the connection is closed
        ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1);
        if (n < 0) {
            die("read()");
        }
        printf("server says: %s\n\n", rbuf);

        close(fd);
    }

    return 0;
}

void die(const char* msg){
    perror(msg);
    exit(1);
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
