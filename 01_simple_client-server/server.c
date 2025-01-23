#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

static void handle(int);
void die(const char*);

int main(){
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1){
        printf("Error creating socket\n");
        return 1;
    }

    // Set the socket to reuse the address
    int opt = 1;
    // SOL_SOCKET = socket level, SO_REUSEADDR = reuse address
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
        if(confd, 0){
            continue;
        }

        handle(confd);
        close(confd);
    }


}

void die(const char* msg){
    perror(msg);
    exit(1);
}


static void handle(int fd){
    char buf[1024];
    ssize_t n = read(fd, buf, sizeof(buf));
    if(n < 0){
        die("read()");
    }
    buf[n] = '\0';
    printf("Received: %s\n", buf);

    char wbuf[] = "Hello, client!";
    write(fd, wbuf, sizeof(wbuf));
}

