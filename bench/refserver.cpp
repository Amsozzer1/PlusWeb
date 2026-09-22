// Control server: a single-threaded epoll loop that returns a canned response.
// It does no routing and no parsing, so its throughput is the ceiling imposed by
// this machine, the loopback interface and the load generator. Any PlusWeb
// number has to be read against this, otherwise a slow harness looks like a slow
// framework.
//
// usage: refserver <port>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

static const char kResp[] =
    "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
    "Connection: keep-alive\r\nContent-Length: 5\r\n\r\nhello";

int main(int argc, char** argv) {
    int port = argc > 1 ? atoi(argv[1]) : 18081;
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    int one = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = INADDR_ANY;
    a.sin_port = htons(port);
    if (bind(lfd, (sockaddr*)&a, sizeof(a)) < 0) { perror("bind"); return 1; }
    listen(lfd, 512);
    fcntl(lfd, F_SETFL, O_NONBLOCK);

    int ep = epoll_create1(0);
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = lfd;
    epoll_ctl(ep, EPOLL_CTL_ADD, lfd, &ev);

    printf("READY %d\n", port);
    fflush(stdout);

    std::unordered_map<int, std::string> bufs;
    epoll_event events[256];
    char rb[65536];
    while (true) {
        int n = epoll_wait(ep, events, 256, -1);
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == lfd) {
                int c;
                while ((c = accept(lfd, nullptr, nullptr)) >= 0) {
                    fcntl(c, F_SETFL, O_NONBLOCK);
                    setsockopt(c, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
                    epoll_event e{};
                    e.events = EPOLLIN;
                    e.data.fd = c;
                    epoll_ctl(ep, EPOLL_CTL_ADD, c, &e);
                    bufs[c] = "";
                }
                continue;
            }
            ssize_t r = recv(fd, rb, sizeof(rb), 0);
            if (r <= 0) {
                epoll_ctl(ep, EPOLL_CTL_DEL, fd, nullptr);
                close(fd);
                bufs.erase(fd);
                continue;
            }
            std::string& b = bufs[fd];
            b.append(rb, r);
            size_t pos, replies = 0;
            while ((pos = b.find("\r\n\r\n")) != std::string::npos) {
                b.erase(0, pos + 4);
                replies++;
            }
            for (size_t k = 0; k < replies; k++) {
                if (send(fd, kResp, sizeof(kResp) - 1, MSG_NOSIGNAL) < 0) break;
            }
        }
    }
}
