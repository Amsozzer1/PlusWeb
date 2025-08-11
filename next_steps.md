# PlusWeb - Next Steps for Complete MVP

## Current Status ✅
- Dynamic routing with parameters (`:id`, `:userId/posts/:postId`)
- GET method working with route registration
- Response helpers implemented
- Segment-based trie for efficient path matching
- Clean Express.js-like API

## Missing for Complete MVP

### 1. HTTP Method Support
**Goal:** Support POST, PUT, DELETE methods alongside GET

**Tasks:**
- [ ] Modify RouteRegistry to store method information
  - Option A: Store as `"METHOD:path"` key
  - Option B: Add method field to route storage
  - Option C: Separate method filtering before trie lookup
- [ ] Implement `server.POST()`, `server.PUT()`, `server.DELETE()` methods
- [ ] Update route matching to consider method + path
- [ ] Test all four methods with same path (different handlers)

### 2. Request Body Parsing
**Goal:** Parse POST/PUT request bodies

**Tasks:**
- [ ] Read `Content-Length` header during request parsing
- [ ] Read body bytes from socket after headers (`\r\n\r\n`)
- [ ] Store raw body in `HttpRequest.body`
- [ ] Handle case where Content-Length is missing/zero
- [ ] Test with JSON and form data

**Technical Notes:**
- Body reading is same for all methods
- For MVP: store as string, let handlers parse
- Content-Type handling can come later

### 3. Enhanced Request Parsing
**Goal:** Properly parse all HTTP methods and bodies

**Tasks:**
- [ ] Update HTTP request parser to extract method from first line
- [ ] Ensure method is stored in `HttpRequest.method`
- [ ] Test method detection with curl/browser requests
- [ ] Verify body parsing works with POST data

### 4. Testing & Validation
**Goal:** Ensure all methods work correctly

**Test Cases:**
- [ ] `GET /users` - no body
- [ ] `POST /users` - with JSON body
- [ ] `PUT /users/:id` - with update data
- [ ] `DELETE /users/:id` - no body, parameter extraction
- [ ] Same path with different methods route to different handlers

## Success Criteria
When complete, this should work:

```cpp
server.GET("/users", getUsersHandler);
server.POST("/users", createUserHandler);
server.PUT("/users/:id", updateUserHandler);
server.DELETE("/users/:id", deleteUserHandler);
```

With handlers receiving:
- Correct method in `req.method`
- Parameters in `req.params["id"]`
- Body content in `req.body` (for POST/PUT)

## Implementation Order
1. **Method storage** - decide how to handle method+path in registry
2. **Body parsing** - read Content-Length and body bytes
3. **Method registration** - implement POST/PUT/DELETE methods
4. **Integration testing** - verify all methods work

## Future (Post-MVP)
- Query string parsing (`?name=value`)
- Content-Type specific parsing (JSON, form data)
- Middleware system
- Error handling (404, 500)
- Static file serving

## Files to Modify
- `RouteRegistry.h/cpp` - method storage
- `HttpServer.h/cpp` - new method registration functions
- `HttpRequest.h/cpp` - ensure method field exists
- Request parsing logic - body reading

**Goal: Complete HTTP method support for full Express.js-like functionality**