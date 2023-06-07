#include "../c++/Controller.h"
#include "../c++/ImageCache.h"
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpUtils.h>
#include <gtest/gtest.h>

// Test fixture class for the HelloWorldController
class HelloWorldControllerTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Create an instance of the controller for testing
    controller = std::make_shared<HelloWorldController>();

    // Set up a dummy HttpRequest and callback for testing
    request = drogon::HttpRequest::newHttpRequest();
    callback = [](const drogon::HttpResponsePtr &) {};
  }

  void TearDown() override {
    // Clean up resources after each test
    controller.reset();
    request.reset();
  }

  std::shared_ptr<HelloWorldController> controller;
  drogon::HttpRequestPtr request;
  std::function<void(const drogon::HttpResponsePtr &)> callback;
};

// Test case for the sayHello() method
TEST_F(HelloWorldControllerTest, SayHelloTest) {
  // Invoke the sayHello() method
  controller->sayHello(request, callback);

  // Get the response from the callback
  const drogon::HttpResponsePtr &response =
      callback.target<std::function<void(const drogon::HttpResponsePtr &)>>()();

  // Assert that the response body matches the expected value
  EXPECT_EQ(response->getBody(),
            "Welcome to Kalz' High Performance Web Server");
}

// Test case for the resizeAndReturnImage() method
TEST_F(HelloWorldControllerTest, ResizeAndReturnImageTest) {
  // Create a JSON payload for the request body
  std::string jsonPayload = R"({
    "imageLink": "https://example.com/image.jpg",
    "width": 800,
    "height": 600,
    "retainFormat": true
  })";

  // Set the request method, body, and content type
  request->setMethod(drogon::HttpMethod::Post);
  request->setBody(jsonPayload);
  request->addHeader("Content-Type", "application/json");

  // Invoke the resizeAndReturnImage() method
  controller->resizeAndReturnImage(request, callback);

  // Get the response from the callback
  const drogon::HttpResponsePtr &response =
      callback.target<std::function<void(const drogon::HttpResponsePtr &)>>()();

  // Assert that the response status code is 200 (OK)
  EXPECT_EQ(response->getStatusCode(), drogon::HttpStatusCode::k200OK);
}

int main(int argc, char **argv) {
  // Initialize the Google Test framework
  ::testing::InitGoogleTest(&argc, argv);

  // Run the tests
  return RUN_ALL_TESTS();
}
