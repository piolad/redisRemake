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

    std::vector<Conn *> conns;          // actually stores our connections from application logic
    std::vector<struct pollfd> pollfds; // for poll - recreated in each iteration

    while (true)
    {
        // recreate pollfds
        pollfds.clear();
        pollfds.push_back({fd, POLLIN, 0}); //always add our listening fd

        // optionally check if any Conn has anything to do
        for (Conn *conn : conns)
        {
            if (!conn) continue;

            if (conn->want_read)
                pollfds.push_back({conn->fd, POLLIN | POLLERR, 0});

            if (conn->want_write)
                pollfds.push_back({conn->fd, POLLOUT | POLLERR, 0});
            
        }


        // call poll - this blocks if we have nothing for any of our pollfds
        // poll updates revents - informing for which fd we have something to do now
        int rv = poll(pollfds.data(), pollfds.size(), -1);
        if (rv < 0 && errno == EINTR) continue;
        if (rv < 0) die("poll()");

        // 0 - the listening socket
        if (pollfds[0].revents)
        {
            if(Conn* conn = handle_accept(fd)){
                if (conns.size() < (size_t)conn->fd)
                    conns.resize(conn->fd + 1);
                conns[conn->fd] = conn;
            }
        }

        // iterate over rest of poll_fds
        for (size_t i = 1; i < pollfds.size(); i++)
        {
            uint32_t revents = pollfds[i].revents;
            if(revents == 0) continue; // nothing is to be done - skip
            // otherwise - execute actions the fd wants - read, write or close

            Conn *conn = conns[pollfds[i].fd];

            if(revents & POLLIN){
                assert(conn->want_read);
                handle_read(conn);
            }

            if(revents & POLLOUT){
                assert(conn->want_write);
                handle_write(conn);
            }

            // close conneciton which error or want to be closed
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

// accept a connection from listening fd - create a new FD and a Conn object
static Conn* handle_accept(int fd){
    struct sockaddr_in client_addr = {};
    socklen_t addrlen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr*) &client_addr, &addrlen);

    if (connfd < 0) return NULL;

    // set fd to nonblocking mode
    fcntl(connfd, F_SETFL, fcntl(connfd, F_GETFL, 0) | O_NONBLOCK);
    
    Conn* conn = new Conn();
    conn->fd = connfd;
    conn-> want_read = true;
    // fprintf(stderr, "newconn\n");
    return conn;
}

// read from available data and try to use it if possible
static void handle_read(Conn* conn){
    // printf("handle read\n");
    uint8_t buf[64*1024];
    ssize_t rv = read(conn->fd, buf, sizeof(buf));

    if(rv <= 0) {
        conn->want_close = true;
        return;
    }

    buf_append(conn->read_buf, buf, (size_t) rv);
    while(try_one_request(conn)){}

    if(conn->write_buf.size() > 0){
        // we have a response to return
        conn->want_read = false;
        conn->want_write = true;
        handle_write(conn);
        return;
    }
}

// write to the buffer and remove itself if done
static void handle_write(Conn* conn){
    //  printf("handle_write!\n");
    assert(conn->write_buf.size() > 0);
    ssize_t rv = write(conn->fd, &conn->write_buf[0], conn->write_buf.size());
    if(rv < 0 && errno == EAGAIN)
        return;
    
    if(rv <0){
        // error
        conn->want_close = true;
        return;
    }

    // printf("written %d", rv);

    // otherwise - remove written data from Conn buffer
    buf_consume(conn->write_buf, (size_t) rv);

    // if 0 - we are done with writing
    if(conn->write_buf.size() == 0){
        conn->want_read = true;
        conn->want_write = false;
    }
    // otherwise - we still ahgve something to write
}



static bool try_one_request(Conn *conn){
    // if we don't yet have the data length - skip
    if(conn->read_buf.size() < 4) return false;

    uint32_t len = 0;
    memcpy(&len, conn->read_buf.data(), 4);

    // we have too much data -error
    if(len > MAX_MSG_SIZE + 4){
        conn->want_close = true;
        return false;
    }

    // if we dont have all data - wait
    if(4+len > conn->read_buf.size())
        return false;

    // otherwise - we can continue with application logic

    const uint8_t* request = &conn->read_buf[4];


    // write the response
    std::string payload = "ok. msg_len=" + std::to_string(len);
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