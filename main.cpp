#ifndef MYHEADER_H
#define MYHEADER_H
#include "include/PlusWeb/HttpRequest.h"
#include "include/PlusWeb/HttpResponse.h"
#include "include/PlusWeb/HttpServer.h"
#endif


int main() {
      

    HttpServer server(8084);

    server.GET("/", [](HttpRequest& req, HttpResponse& res) {
        res.status(200).send("<html><body>HELLO</body></html>");
    });

    server.GET("/me/:id", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        
        nlohmann::json user_data = {
            {"name", "ahmed"},
            {"id", userId}
        };
        
        res.status(200).send(user_data);
    });

    server.GET("/users", [](HttpRequest& req, HttpResponse& res) {
        nlohmann::json users = {
            {{"name", "sozzer"}, {"id", 1}},
            {{"name", "ahmed"}, {"id", 2}},
            {{"name", "john"}, {"id", 3}}
        };
        
        res.status(200).send(users);
    });

    server.GET("/users/:id", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        std::cout << "Getting user with ID: " << userId << std::endl;
        
        nlohmann::json user = {
            {"name", "ahmed"},
            {"id", userId},
            {"email", "ahmed@example.com"},
            {"created_at", "2025-01-01T00:00:00Z"}
        };
        
        res.status(200).send(user);
    });

    server.GET("/users/:id/policy/:policyId", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        std::string policyId = req.params["policyId"];
        
        nlohmann::json policy_data = {
            {"userId", userId},
            {"policyId", policyId},
            {"policyName", "Standard Policy"},
            {"permissions", {"read", "write"}},
            {"active", true}
        };
        
        res.status(200).send(policy_data);
    });

    server.GET("/names/:name", [](HttpRequest& req, HttpResponse& res) {
        std::string name = req.params["name"];
        
        std::string html_response = 
            "<html>"
            "<head><title>Hello " + name + "</title></head>"
            "<body>"
            "<h1>Welcome, " + name + "!</h1>"
            "<p>Method used: " + req.method + "</p>"
            "</body>"
            "</html>";
        
        res.status(200).send(html_response);
    });

    server.DELETE("/names/:name", [](HttpRequest& req, HttpResponse& res) {
        std::string name = req.params["name"];
        
        // Log the request body for debugging
        std::cout << "DELETE request body: " << req.body.getRaw() << std::endl;
        
        nlohmann::json delete_response = {
            {"deleted", true},
            {"name", name},
            {"message", "Name '" + name + "' has been successfully deleted"},
            {"timestamp", "2025-01-01T00:00:00Z"}
        };
        
        res.status(200).send(delete_response);
    });

    server.GET("/users/:id/data", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        
        nlohmann::json user_data = {
            {"id", userId},
            {"data", "Some Data here"},
            {"profile", {
                {"preferences", "dark_mode"},
                {"language", "en"},
                {"timezone", "UTC"}
            }},
            {"last_login", "2025-01-01T12:00:00Z"}
        };
        
        res.status(200).send(user_data);
    });

    server.POST("/users/:id", [](HttpRequest& req, HttpResponse& res) {
        std::string userId = req.params["id"];
        
        // Log request data for debugging
        if (req.body.isJson()) {
            std::cout << "POST request body: " << req.body.getJson() << std::endl;
            
            // Check for specific fields
            auto json_body = req.body.getJson();
            if (json_body.contains("abc")) {
                std::cout << "ABC field value: " << json_body["abc"] << std::endl;
            }
        }
        
        nlohmann::json response_data = {
            {"id", userId},
            {"status", "updated"},
            {"message", "User data updated successfully"},
            {"received_data", req.body.isJson() ? req.body.getJson() : nlohmann::json::object()},
            {"timestamp", "2025-01-01T00:00:00Z"}
        };
        
        res.status(200).send(response_data);
    });


    server.GET("/health", [](HttpRequest& req, HttpResponse& res) {
        nlohmann::json health_status = {
            {"status", "healthy"},
            {"timestamp", "2025-01-01T00:00:00Z"},
            {"uptime", "24h 15m 32s"},
            {"version", "1.0.0"}
        };
        
        res.status(200).send(health_status);
    });

    server.GET("/api/info", [](HttpRequest& req, HttpResponse& res) {
        nlohmann::json api_info = {
            {"name", "PlusWeb API"},
            {"version", "1.0.0"},
            {"description", "A modern C++ web framework"},
            {"endpoints", {
                "/", "/users", "/users/:id", "/health"
            }}
        };
        
        res.status(200).send(api_info);
    });

    server.POST("/echo", [](HttpRequest& req, HttpResponse& res) {
        nlohmann::json echo_response = {
            {"method", req.method},
            {"path", req.path},
            {"headers", nlohmann::json::object()}, // You'd populate this from req.headers
            {"body", req.body.isJson() ? req.body.getJson() : nlohmann::json(req.body.getRaw())},
            {"timestamp", "2025-01-01T00:00:00Z"}
        };

        res.status(200).send(echo_response);
    });

    server.GET("/error", [](HttpRequest& req, HttpResponse& res) {
        nlohmann::json error_response = {
            {"error", true},
            {"message", "This is a test error"},
            {"code", "TEST_ERROR"},
            {"timestamp", "2025-01-01T00:00:00Z"}
        };
        
        res.status(500).send(error_response);
    });



    server.serve([](){
        
    });

    return 0;
}
