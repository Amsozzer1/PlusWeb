// Closed-loop epoll HTTP/1.1 load generator.
//
// One request in flight per connection; a reply immediately triggers the next
// request on that connection. Records per-request latency so we can report
// percentiles, and counts connections that never completed a single request
// (that is the interesting number when a server has fewer workers than
// connections).
//
// usage: loadgen <host> <port> <conns> <seconds> <path> [--close] [--warmup N]
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

using Clock = std::chrono::steady_clock;

static double now_s() {
    return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}

struct Conn {
    int fd = -1;
    std::string inbuf;
    double sent_at = 0;
    long done = 0;
    bool writing = false;
    size_t written = 0;
};

// Returns the total length of the first complete HTTP response in buf, or 0.
static size_t response_len(const std::string& buf) {
    size_t hdr_end = buf.find("\r\n\r\n");
    if (hdr_end == std::string::npos) return 0;
    size_t body_start = hdr_end + 4;
    // Content-Length is mandatory for this harness; the server under test always
    // sets it. Header search is case-insensitive-ish but the server emits exactly
    // "Content-Length".
    size_t cl = buf.find("Content-Length:");
    if (cl == std::string::npos || cl > hdr_end) {
        return body_start;  // no body advertised
    }
    long len = strtol(buf.c_str() + cl + 15, nullptr, 10);
    if (buf.size() < body_start + (size_t)len) return 0;
    return body_start + len;
}

int main(int argc, char** argv) {
    if (argc < 6) {
        fprintf(stderr,
                "usage: %s <host> <port> <conns> <seconds> <path> [--close]\n",
                argv[0]);
        return 2;
    }
    const char* host = argv[1];
    int port = atoi(argv[2]);
    int nconns = atoi(argv[3]);
    double duration = atof(argv[4]);
    std::string path = argv[5];
    bool close_mode = false;
    for (int i = 6; i < argc; i++) {
        if (!strcmp(argv[i], "--close")) close_mode = true;
    }

    std::string req = "GET " + path +
                      " HTTP/1.1\r\nHost: bench\r\nAccept: */*\r\nConnection: " +
                      (close_mode ? "close" : "keep-alive") + "\r\n\r\n";

    int ep = epoll_create1(0);
    std::vector<Conn> conns(nconns);
    std::vector<double> lat;
    lat.reserve(1 << 22);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &addr.sin_addr);

    auto connect_one = [&](int i) -> bool {
        Conn& c = conns[i];
        if (c.fd >= 0) { epoll_ctl(ep, EPOLL_CTL_DEL, c.fd, nullptr); close(c.fd); }
        c.fd = socket(AF_INET, SOCK_STREAM, 0);
        if (c.fd < 0) return false;
        int one = 1;
        setsockopt(c.fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
        if (connect(c.fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            close(c.fd); c.fd = -1; return false;
        }
        fcntl(c.fd, F_SETFL, O_NONBLOCK);
        c.inbuf.clear(); c.writing = true; c.written = 0; c.sent_at = now_s();
        epoll_event ev{}; ev.events = EPOLLOUT; ev.data.u32 = i;
        epoll_ctl(ep, EPOLL_CTL_ADD, c.fd, &ev);
        return true;
    };

    int connected = 0;
    for (int i = 0; i < nconns; i++) {
        if (connect_one(i)) connected++;
    }
    if (connected == 0) { fprintf(stderr, "no connections established\n"); return 1; }

    double start = now_s(), deadline = start + duration;
    long completed = 0, errors = 0, reconnects = 0;
    std::vector<epoll_event> events(std::max(64, nconns));

    while (now_s() < deadline) {
        int n = epoll_wait(ep, events.data(), (int)events.size(), 100);
        for (int k = 0; k < n; k++) {
            int i = events[k].data.u32;
            Conn& c = conns[i];
            if (c.fd < 0) continue;

            if (events[k].events & EPOLLOUT) {
                ssize_t w = send(c.fd, req.data() + c.written,
                                 req.size() - c.written, MSG_NOSIGNAL);
                if (w < 0) {
                    if (errno != EAGAIN) { errors++; c.fd = -1; }
                    continue;
                }
                c.written += w;
                if (c.written == req.size()) {
                    c.writing = false;
                    epoll_event ev{}; ev.events = EPOLLIN; ev.data.u32 = i;
                    epoll_ctl(ep, EPOLL_CTL_MOD, c.fd, &ev);
                }
            }
            if (events[k].events & EPOLLIN) {
                char buf[65536];
                ssize_t r = recv(c.fd, buf, sizeof(buf), 0);
                if (r > 0) {
                    c.inbuf.append(buf, r);
                    size_t len;
                    while ((len = response_len(c.inbuf)) > 0 && c.inbuf.size() >= len) {
                        lat.push_back(now_s() - c.sent_at);
                        completed++; c.done++;
                        c.inbuf.erase(0, len);
                        if (close_mode) {
                            if (now_s() < deadline && connect_one(i)) reconnects++;
                            break;
                        }
                        // pipeline the next request on the same connection
                        c.sent_at = now_s();
                        c.written = 0;
                        ssize_t w = send(c.fd, req.data(), req.size(), MSG_NOSIGNAL);
                        if (w < 0) { errors++; break; }
                        c.written = w;
                        if ((size_t)w < req.size()) {
                            epoll_event ev{}; ev.events = EPOLLOUT; ev.data.u32 = i;
                            epoll_ctl(ep, EPOLL_CTL_MOD, c.fd, &ev);
                        }
                        break;
                    }
                } else if (r == 0 || (r < 0 && errno != EAGAIN)) {
                    // server hung up
                    epoll_ctl(ep, EPOLL_CTL_DEL, c.fd, nullptr);
                    close(c.fd); c.fd = -1;
                    if (!close_mode && now_s() < deadline) {
                        if (connect_one(i)) reconnects++; else errors++;
                    }
                }
            }
        }
    }
    double elapsed = now_s() - start;

    int starved = 0;
    for (auto& c : conns) if (c.done == 0) starved++;
    for (auto& c : conns) if (c.fd >= 0) close(c.fd);

    std::sort(lat.begin(), lat.end());
    auto pct = [&](double p) -> double {
        if (lat.empty()) return -1;
        size_t idx = (size_t)(p / 100.0 * (lat.size() - 1));
        return lat[idx] * 1000.0;
    };
    double mean = 0;
    for (double d : lat) mean += d;
    if (!lat.empty()) mean = mean / lat.size() * 1000.0;

    printf("{\"conns\":%d,\"connected\":%d,\"mode\":\"%s\",\"elapsed_s\":%.3f,"
           "\"completed\":%ld,\"rps\":%.1f,\"errors\":%ld,\"reconnects\":%ld,"
           "\"starved_conns\":%d,\"mean_ms\":%.3f,\"p50_ms\":%.3f,\"p90_ms\":%.3f,"
           "\"p99_ms\":%.3f,\"p999_ms\":%.3f,\"max_ms\":%.3f}\n",
           nconns, connected, close_mode ? "close" : "keep-alive", elapsed,
           completed, completed / elapsed, errors, reconnects, starved, mean,
           pct(50), pct(90), pct(99), pct(99.9),
           lat.empty() ? -1 : lat.back() * 1000.0);
    return 0;
}
