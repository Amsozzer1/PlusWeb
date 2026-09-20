#include "HttpParser.h"

#include <PlusWeb/utils.h>

#include <llhttp.h>

#include <memory>

namespace {

// Splits "/path?a=1&b=2" into a decoded path and a query map.
void applyTarget(const std::string& target, HttpRequest& req) {
    const std::string decoded = Utils::url_decode(target);
    const size_t q = decoded.find('?');
    req.path = decoded.substr(0, q);
    if (req.path.empty()) {
        req.path = "/";
    }
    if (q == std::string::npos) {
        return;
    }
    for (const std::string& pair : Utils::split(decoded.substr(q + 1).c_str(), "&")) {
        const size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            req.query[pair.substr(0, eq)] = pair.substr(eq + 1);
        }
    }
}

}  // namespace

struct HttpParser::Impl {
    llhttp_t parser;
    llhttp_settings_t settings;

    // State for the request currently being parsed.
    std::string target;
    std::string field;
    std::string value;
    std::string body;
    std::map<std::string, std::string> headers;
    bool readingValue = false;

    const RequestHandler* handler = nullptr;
    std::string failure;

    static Impl* of(llhttp_t* p) { return static_cast<Impl*>(p->data); }

    void commitHeader() {
        if (!field.empty()) {
            headers[field] = value;
        }
        field.clear();
        value.clear();
    }

    void resetRequest() {
        target.clear();
        field.clear();
        value.clear();
        body.clear();
        headers.clear();
        readingValue = false;
    }

    static int onUrl(llhttp_t* p, const char* at, size_t len) {
        of(p)->target.append(at, len);
        return 0;
    }

    // Header names and values can each arrive in several chunks, so a value
    // chunk following a field chunk is what marks the end of the name.
    static int onHeaderField(llhttp_t* p, const char* at, size_t len) {
        Impl* i = of(p);
        if (i->readingValue) {
            i->commitHeader();
            i->readingValue = false;
        }
        i->field.append(at, len);
        return 0;
    }

    static int onHeaderValue(llhttp_t* p, const char* at, size_t len) {
        Impl* i = of(p);
        i->readingValue = true;
        i->value.append(at, len);
        return 0;
    }

    static int onHeadersComplete(llhttp_t* p) {
        of(p)->commitHeader();
        return 0;
    }

    static int onBody(llhttp_t* p, const char* at, size_t len) {
        of(p)->body.append(at, len);
        return 0;
    }

    static int onMessageComplete(llhttp_t* p) {
        Impl* i = of(p);

        HttpRequest request;
        request.method = llhttp_method_name(static_cast<llhttp_method_t>(p->method));
        request.protocol = "HTTP/" + std::to_string(p->http_major) + "." +
                           std::to_string(p->http_minor);
        request.headers = i->headers;
        applyTarget(i->target, request);

        if (!i->body.empty()) {
            try {
                if (request.headers["Content-Type"] == "application/json") {
                    request.body.setJson(nlohmann::json::parse(i->body));
                } else {
                    request.body.setText(i->body);
                }
            } catch (const std::exception& e) {
                i->failure = std::string("invalid JSON body: ") + e.what();
                return HPE_USER;
            }
        }

        const bool keepAlive = llhttp_should_keep_alive(p) != 0;
        i->resetRequest();

        // A handler must not throw into llhttp's C frames.
        try {
            (*i->handler)(request, keepAlive);
        } catch (const std::exception& e) {
            i->failure = std::string("handler threw: ") + e.what();
            return HPE_USER;
        }
        return 0;
    }
};

HttpParser::HttpParser() : impl(std::make_shared<Impl>()) {
    llhttp_settings_init(&impl->settings);
    impl->settings.on_url = &Impl::onUrl;
    impl->settings.on_header_field = &Impl::onHeaderField;
    impl->settings.on_header_value = &Impl::onHeaderValue;
    impl->settings.on_headers_complete = &Impl::onHeadersComplete;
    impl->settings.on_body = &Impl::onBody;
    impl->settings.on_message_complete = &Impl::onMessageComplete;

    llhttp_init(&impl->parser, HTTP_REQUEST, &impl->settings);
    impl->parser.data = impl.get();
}

bool HttpParser::execute(const char* data, size_t len, const RequestHandler& onRequest) {
    impl->handler = &onRequest;
    impl->failure.clear();

    const llhttp_errno_t rc = llhttp_execute(&impl->parser, data, len);

    impl->handler = nullptr;
    if (rc == HPE_OK) {
        return true;
    }

    errorReason = !impl->failure.empty()
                      ? impl->failure
                      : std::string(llhttp_errno_name(rc)) + ": " +
                            (impl->parser.reason ? impl->parser.reason : "");
    return false;
}
