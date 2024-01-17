#include "../src/server/imghttp.h"
#include <curl/curl.h>
#include <gtest/gtest.h>
#include <thread>

namespace {

std::string performPostRequest(const std::string &url,
                               const std::string &postData) {
  CURL *curl = curl_easy_init();
  std::string responseString;
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L);

    curl_easy_setopt(
        curl, CURLOPT_WRITEFUNCTION,
        +[](void *contents, size_t size, size_t nmemb,
            std::string *s) -> size_t {
          s->append(static_cast<char *>(contents), size * nmemb);
          return size * nmemb;
        });
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseString);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
  }
  return responseString;
}

class ImgHTTPTest : public ::testing::Test {
protected:
  static void SetUpTestSuite() {
    serverThread = std::thread(
        []() { ImgHttp::server("0.0.0.0", 80, "test_database.db").run(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  static void TearDownTestSuite() {
    drogon::app().quit();
    if (serverThread.joinable()) {
      serverThread.join();
    }
  }

  static std::thread serverThread;
};

std::thread ImgHTTPTest::serverThread;

TEST_F(ImgHTTPTest, ResizeImageTest) {
  std::string url = "http://localhost/resize";
  std::string postData = "{\"imageLink\": "
                         "\"http://www.wagnerrp.com/images/backgrounds/1680/"
                         "history_of_things_to_come.jpg\", \"width\": 100, "
                         "\"height\": 100, \"retainFormat\": true}";
  std::string response = performPostRequest(url, postData);
  ASSERT_FALSE(response.empty());
}

} // namespace
