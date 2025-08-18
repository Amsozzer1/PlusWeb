#pragma once

#include <functional>

// Forward Declarations
class HttpRequest;
class HttpResponse;

using RouteHandler = std::function<void(HttpRequest&, HttpResponse&)>;
using NextFunction = std::function<void()>;
using MiddlewareFunction = std::function<void(HttpRequest&, HttpResponse&, NextFunction)>;


