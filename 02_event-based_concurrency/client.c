#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static void do_something(int);
void die(const char*);

int main(){
    int fd, rv;
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);  // 127.0.0.1

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
        if (msg[strlen(msg) - 1] == '\n') {
            msg[strlen(msg) - 1] = '\0';
        }
        write(fd, msg, strlen(msg));

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
