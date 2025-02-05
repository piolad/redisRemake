#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include <poll.h>
#include <errno.h>


static void handle(int);
void die(const char*);

struct Conn{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;

    std::vector<uint8_t> read_buf;
    std::vector<uint8_t> write_buf;    
};

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

    std::vector<Conn*> conns;
    std::vector<struct pollfd> pollfds;

    while(true){
        pollfds.clear();

        pollfds.push_back({fd, POLLIN, 0});
        
        for(Conn* conn : conns){
            if(!conn){
                continue;
            }

            if(conn->want_read){
                pollfds.push_back({conn->fd, POLLIN | POLLERR, 0});
            }
            if(conn->want_write){
                pollfds.push_back({conn->fd, POLLOUT | POLLERR, 0});
            }


            int rv = poll(pollfds.data(), pollfds.size(), -1);
            if(rv < 0 && errno == EINTR){
                continue;
            }
            if(rv < 0){
                die("poll()");
            }

            
            
        }
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

