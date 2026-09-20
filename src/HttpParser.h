#pragma once

#include <PlusWeb/HttpRequest.h>

#include <functional>
#include <map>
#include <string>

// One HTTP request parser per connection, backed by llhttp.
//
// Internal header: it is not installed, so llhttp.h stays out of the public
// headers. llhttp handles framing as well as parsing, so pipelined requests,
// chunked bodies and requests split across packets all work.
class HttpParser {
public:
    using RequestHandler = std::function<void(HttpRequest&, bool keepAlive)>;

    // Request line + header bytes accepted before a request is rejected. Node
    // uses 16 KiB; matching it keeps behaviour predictable for clients.
    static constexpr size_t kMaxHeaderBytes = 16 * 1024;

    explicit HttpParser(size_t maxHeaderBytes = kMaxHeaderBytes);

    // Returns false if the bytes are not valid HTTP; the caller should then
    // reply with status() and close.
    bool execute(const char* data, size_t len, const RequestHandler& onRequest);

    const std::string& error() const { return errorReason; }

    // Status to answer the failure with: 431 when the header limit was hit,
    // 400 otherwise.
    int status() const { return errorStatus; }

private:
    struct Impl;
    std::shared_ptr<Impl> impl;

    std::string errorReason;
    int errorStatus = 400;
};
