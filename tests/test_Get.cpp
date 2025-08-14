#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include "json.hpp"
#include "../include/PlusWeb/HttpServer.h"

using json = nlohmann::json;

// Helper struct to capture HTTP response
struct HttpTestResponse {
    std::string body;
    std::string headers;
    long status_code = 0;
    std::string content_type;
};

// Callback function for curl to write response data
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append((char*)contents, total_size);
    return total_size;
}

// Callback function for curl to write header data
static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, std::string* headers) {
    size_t total_size = size * nitems;
    headers->append(buffer, total_size);
    return total_size;
}

class HttpServerTest : public ::testing::Test {
protected:
    static HttpServer* server;
    static std::thread* server_thread;
    static const int TEST_PORT = 8085;
    static const std::string BASE_URL;

    static void SetUpTestSuite() {
        // Initialize curl
        curl_global_init(CURL_GLOBAL_DEFAULT);
        
        // Create and configure server
        server = new HttpServer(TEST_PORT);
        setupRoutes();
        
        // Start server in separate thread
        server_thread = new std::thread([]() {
            while (true) {
                server->handleClient();
            }
        });
        
        // Give server time to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    static void TearDownTestSuite() {
        // Note: In production, you'd want proper server shutdown
        delete server_thread;
        delete server;
        curl_global_cleanup();
    }

    static void setupRoutes() {
        server->GET("/", [](HttpRequest& req, HttpResponse& res) {
            res.status(200).send("<html><body>HELLO</body></html>");
        });

        server->GET("/me/:id", [](HttpRequest& req, HttpResponse& res) {
            std::string userId = req.params["id"];
            json user_data = {
                {"name", "ahmed"},
                {"id", userId}
            };
            res.status(200).send(user_data);
        });

        server->GET("/users", [](HttpRequest& req, HttpResponse& res) {
            json users = {
                {{"name", "sozzer"}, {"id", 1}},
                {{"name", "ahmed"}, {"id", 2}},
                {{"name", "john"}, {"id", 3}}
            };
            res.status(200).send(users);
        });

        server->GET("/users/:id", [](HttpRequest& req, HttpResponse& res) {
            std::string userId = req.params["id"];
            json user = {
                {"name", "ahmed"},
                {"id", userId},
                {"email", "ahmed@example.com"},
                {"created_at", "2025-01-01T00:00:00Z"}
            };
            res.status(200).send(user);
        });

        server->GET("/users/:id/policy/:policyId", [](HttpRequest& req, HttpResponse& res) {
            std::string userId = req.params["id"];
            std::string policyId = req.params["policyId"];
            json policy_data = {
                {"userId", userId},
                {"policyId", policyId},
                {"policyName", "Standard Policy"},
                {"permissions", {"read", "write"}},
                {"active", true}
            };
            res.status(200).send(policy_data);
        });

        server->GET("/names/:name", [](HttpRequest& req, HttpResponse& res) {
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

        server->GET("/health", [](HttpRequest& req, HttpResponse& res) {
            json health_status = {
                {"status", "healthy"},
                {"timestamp", "2025-01-01T00:00:00Z"},
                {"uptime", "24h 15m 32s"},
                {"version", "1.0.0"}
            };
            res.status(200).send(health_status);
        });

        server->GET("/error", [](HttpRequest& req, HttpResponse& res) {
            json error_response = {
                {"error", true},
                {"message", "This is a test error"},
                {"code", "TEST_ERROR"},
                {"timestamp", "2025-01-01T00:00:00Z"}
            };
            res.status(500).send(error_response);
        });
    }

    // Helper method to make HTTP GET request
    HttpTestResponse makeGetRequest(const std::string& path) {
        CURL* curl;
        CURLcode res;
        HttpTestResponse response;

        curl = curl_easy_init();
        if (curl) {
            std::string url = BASE_URL + path;
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

            res = curl_easy_perform(curl);
            
            if (res == CURLE_OK) {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.status_code);
                
                // Extract Content-Type from headers
                size_t content_type_pos = response.headers.find("Content-Type:");
                if (content_type_pos != std::string::npos) {
                    size_t start = content_type_pos + 13; // "Content-Type:" length
                    size_t end = response.headers.find("\r\n", start);
                    response.content_type = response.headers.substr(start, end - start);
                    
                    // Trim whitespace
                    response.content_type.erase(0, response.content_type.find_first_not_of(" \t"));
                    response.content_type.erase(response.content_type.find_last_not_of(" \t") + 1);
                }
            }

            curl_easy_cleanup(curl);
        }

        return response;
    }

    // Helper to parse JSON response
    json parseJsonResponse(const HttpTestResponse& response) {
        try {
            return json::parse(response.body);
        } catch (const std::exception& e) {
            ADD_FAILURE() << "Failed to parse JSON: " << e.what() << "\nBody: " << response.body;
            return json();
        }
    }
};

// Static member definitions
HttpServer* HttpServerTest::server = nullptr;
std::thread* HttpServerTest::server_thread = nullptr;
const std::string HttpServerTest::BASE_URL = "http://localhost:8085";

// ===== TEST CASES =====

TEST_F(HttpServerTest, TestHomePage) {
    auto response = makeGetRequest("/");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "text/html; charset=utf-8");
    EXPECT_EQ(response.body, "<html><body>HELLO</body></html>");
    EXPECT_GT(response.body.length(), 0);
}

TEST_F(HttpServerTest, TestGetUserById) {
    auto response = makeGetRequest("/me/12345");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_EQ(json_response["name"], "ahmed");
    EXPECT_EQ(json_response["id"], "12345");
}

TEST_F(HttpServerTest, TestGetAllUsers) {
    auto response = makeGetRequest("/users");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_TRUE(json_response.is_array());
    EXPECT_EQ(json_response.size(), 3);
    
    // Check first user
    EXPECT_EQ(json_response[0]["name"], "sozzer");
    EXPECT_EQ(json_response[0]["id"], 1);
    
    // Check second user
    EXPECT_EQ(json_response[1]["name"], "ahmed");
    EXPECT_EQ(json_response[1]["id"], 2);
}

TEST_F(HttpServerTest, TestGetSpecificUser) {
    auto response = makeGetRequest("/users/789");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_EQ(json_response["name"], "ahmed");
    EXPECT_EQ(json_response["id"], "789");
    EXPECT_EQ(json_response["email"], "ahmed@example.com");
    EXPECT_EQ(json_response["created_at"], "2025-01-01T00:00:00Z");
}

TEST_F(HttpServerTest, TestGetUserPolicy) {
    auto response = makeGetRequest("/users/123/policy/policy-456");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_EQ(json_response["userId"], "123");
    EXPECT_EQ(json_response["policyId"], "policy-456");
    EXPECT_EQ(json_response["policyName"], "Standard Policy");
    EXPECT_TRUE(json_response["active"]);
    
    // Check permissions array
    EXPECT_TRUE(json_response["permissions"].is_array());
    EXPECT_EQ(json_response["permissions"].size(), 2);
    EXPECT_EQ(json_response["permissions"][0], "read");
    EXPECT_EQ(json_response["permissions"][1], "write");
}

TEST_F(HttpServerTest, TestGetNameWithHTMLResponse) {
    auto response = makeGetRequest("/names/TestUser");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "text/html; charset=utf-8");
    
    // Check HTML content
    EXPECT_NE(response.body.find("<title>Hello TestUser</title>"), std::string::npos);
    EXPECT_NE(response.body.find("<h1>Welcome, TestUser!</h1>"), std::string::npos);
    EXPECT_NE(response.body.find("Method used: GET"), std::string::npos);
}

TEST_F(HttpServerTest, TestHealthEndpoint) {
    auto response = makeGetRequest("/health");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_EQ(json_response["status"], "healthy");
    EXPECT_EQ(json_response["version"], "1.0.0");
    EXPECT_TRUE(json_response.contains("timestamp"));
    EXPECT_TRUE(json_response.contains("uptime"));
}

TEST_F(HttpServerTest, TestErrorEndpoint) {
    auto response = makeGetRequest("/error");
    
    EXPECT_EQ(response.status_code, 500);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_TRUE(json_response["error"]);
    EXPECT_EQ(json_response["message"], "This is a test error");
    EXPECT_EQ(json_response["code"], "TEST_ERROR");
}

TEST_F(HttpServerTest, TestNotFoundRoute) {
    auto response = makeGetRequest("/nonexistent");
    
    EXPECT_EQ(response.status_code, 404);
    EXPECT_EQ(response.content_type, "application/json");
    
    auto json_response = parseJsonResponse(response);
    EXPECT_EQ(json_response["error"], "Not Found");
    EXPECT_EQ(json_response["path"], "/nonexistent");
    EXPECT_EQ(json_response["method"], "GET");
}

TEST_F(HttpServerTest, TestParameterExtraction) {
    // Test with special characters in parameters
    auto response = makeGetRequest("/names/John%20Doe");
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.content_type, "text/html; charset=utf-8");
    
    // URL decoding should happen on server side
    // This test verifies the parameter extraction works
    EXPECT_GT(response.body.length(), 0);
}

TEST_F(HttpServerTest, TestResponseHeaders) {
    auto response = makeGetRequest("/users");
    
    // Check that Content-Length header is present
    EXPECT_NE(response.headers.find("Content-Length:"), std::string::npos);
    
    // Check that Connection header is present
    EXPECT_NE(response.headers.find("Connection:"), std::string::npos);
}

TEST_F(HttpServerTest, TestConcurrentRequests) {
    const int NUM_CONCURRENT = 5;
    std::vector<std::thread> threads;
    std::vector<HttpTestResponse> responses(NUM_CONCURRENT);
    
    // Launch concurrent requests
    for (int i = 0; i < NUM_CONCURRENT; ++i) {
        threads.emplace_back([this, i, &responses]() {
            responses[i] = makeGetRequest("/users/" + std::to_string(i + 100));
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all responses
    for (int i = 0; i < NUM_CONCURRENT; ++i) {
        EXPECT_EQ(responses[i].status_code, 200);
        EXPECT_EQ(responses[i].content_type, "application/json");
        
        auto json_response = parseJsonResponse(responses[i]);
        EXPECT_EQ(json_response["id"], std::to_string(i + 100));
    }
}

// ===== MAIN FUNCTION =====
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}