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

    HttpParser();

    // Returns false if the bytes are not valid HTTP; the caller should then
    // reply 400 and close.
    bool execute(const char* data, size_t len, const RequestHandler& onRequest);

    const std::string& error() const { return errorReason; }

private:
    struct Impl;
    std::shared_ptr<Impl> impl;

    std::string errorReason;
};
