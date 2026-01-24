#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vector>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <assert.h>
#include<map>

#define MAX_MSG_SIZE 32 << 20

struct Conn
{
    int fd;
    bool want_read;
    bool want_write;
    bool want_close;

    std::vector<uint8_t> read_buf;
    std::vector<uint8_t> write_buf;
};

int fd = -1;
int rv;
std::map<std::string, std::string> kv = {{"TEST", "hello"}};


static void handle(int);
void die(const char *);
static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);
static void buf_consume(std::vector<uint8_t> &buf, size_t n);
static bool try_one_request(Conn *conn);
static Conn* handle_accept(int fd);
static void handle_write(Conn* conn);
static void handle_read(Conn* conn);
void networking_setup();

int main()
{
    networking_setup();

    std::vector<Conn *> conns;
    std::vector<struct pollfd> pollfds;

    while (true)
    {
        pollfds.clear();
        pollfds.push_back({fd, POLLIN, 0});
        for (Conn *conn : conns)
        {
            if (!conn) continue;
            if (conn->want_read)
                pollfds.push_back({conn->fd, POLLIN | POLLERR, 0});
            if (conn->want_write)
                pollfds.push_back({conn->fd, POLLOUT | POLLERR, 0});
        }

        int rv = poll(pollfds.data(), pollfds.size(), -1);
        if (rv < 0 && errno == EINTR) continue;
        if (rv < 0) die("poll()");

        if (pollfds[0].revents)
        {
            if(Conn* conn = handle_accept(fd)){
                if (conns.size() < (size_t)conn->fd)
                    conns.resize(conn->fd + 1);
                conns[conn->fd] = conn;
            }
        }

        for (size_t i = 1; i < pollfds.size(); i++)
        {
            uint32_t revents = pollfds[i].revents;
            if(revents == 0) continue;
            Conn *conn = conns[pollfds[i].fd];
            if(revents & POLLIN){
                assert(conn->want_read);
                handle_read(conn);
            }
            if(revents & POLLOUT){
                assert(conn->want_write);
                handle_write(conn);
            }
            if ((revents & POLLERR) || conn->want_close)
            {
                (void)close(conn->fd);
                conns[conn->fd] = nullptr;
                delete conn;
            }
        }
    }
}

void die(const char *msg)
{
    perror(msg);
    exit(1);
}

static Conn* handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*) &client_addr, &addrlen);

    if (connfd < 0) return NULL;
    fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK);
    
    Conn* conn = new Conn();
    conn->fd = connfd;
    conn-> want_read = true;

    return conn;
}
static void handle_read(Conn* conn){
    uint8_t buf[64*1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    if(rv <= 0) {
        conn->want_close = true;
        return;
    }

    buf_append(conn->read_buf, buf, (size_t) rv);
    while(try_one_request(conn)){}

    if(conn->write_buf.size() > 0){
        conn->want_read = false;
        conn->want_write = true;
        handle_write(conn);
        return;
    }
}
static void handle_write(Conn* conn){
    assert(conn->write_buf.size() > 0);
    ssize_t rv = write(conn->fd, &conn->write_buf[0], conn->write_buf.size());
    if(rv < 0 && errno == EAGAIN)
        return;
    
    if(rv <0){
        conn->want_close = true;
        return;
    }
    buf_consume(conn->write_buf, (size_t) rv);

    if(conn->write_buf.size() == 0){
        conn->want_read = true;
        conn->want_write = false;
    }
}
static bool try_one_request(Conn *conn){
    // printf("try_one_request\n");
    if(conn->read_buf.size() < 4) return false;

    uint32_t len = 0;
    memcpy(&len, conn->read_buf.data(), 4);

    if(len > MAX_MSG_SIZE + 4){
        conn->want_close = true;
        return false;
    }
    if(4+len > conn->read_buf.size())
        return false;

    std::string command;
    command.reserve(len);
    int i;
    char c;
    for(i = 0; i< len; i++){
        c = char(conn->read_buf[4 + i]);
        if(c == ' '){
            break;
        }
        command.push_back(c);
    }
    std::string args;
    args.reserve(len);
    for(++i; i< len; i++){
        args.push_back(char(conn->read_buf[4 + i]));
    }

    printf("cmd: <%s>; args: <%s>\n", command.c_str(), args.c_str());
    fflush(stdout);

    std::string payload = "ok. msg_len=" + std::to_string(len);
    if(command == "GET"){
        if(kv.count(args)){
            printf("->kv[%s]=%s\n",args.c_str(), kv[args].c_str());
            std::string r = kv[args];
            payload = "OK" + r;
        }else{
            payload = "KO:notfound";
        }
    }else if (command == "SET"){
        // todo: rewrite! here we assume key may not have spaces
        // better to have nested lenght-padding
        std::string::size_type pos = args.find_first_of(' ');
        if (pos == std::string::npos){
            payload = "KO:notfound";
        } else if (pos +1 >= command.size()){
            payload = "KO:malformated";
        } else{
            std::string key = args.substr(0,pos);
            std::string value = args.substr(pos+1);

            kv[key] = value;
            payload = "OK";
        }
    }else if (command == "PING"){
        
    }else if (command == "DEL"){
        
    }

    
    // std::string payload = "ok. msg_len=" + std::to_string(len);
    uint32_t payload_len = payload.size();
    buf_append(conn->write_buf, (const uint8_t*) &payload_len, 4);
    buf_append(conn->write_buf, (const uint8_t*) payload.data(), payload_len);

    buf_consume(conn->read_buf, 4+len);

    return true;

}

static void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
    buf.insert(buf.end(), data, data + len);
}

static void buf_consume(std::vector<uint8_t> &buf, size_t n) {
    buf.erase(buf.begin(), buf.begin() + n);
}


void networking_setup(){
    // AF_INET = IPv4, SOCK_STREAM = TCP
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        printf("Error creating socket\n");
        exit(-1);
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(0);
    int rv = bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("bind()");
    }

    // NEW: make fd non-blocking
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

    rv = listen(fd, SOMAXCONN);
    if (rv)
    {
        die("listen()");
    }
}