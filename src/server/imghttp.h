#ifndef IMGHTTP_H
#define IMGHTTP_H

#include <curl/curl.h>
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <sqlite3.h>
#include <string>

class ImgHttp {

private:
  std::unique_ptr<sqlite3, decltype(&sqlite3_close)> db;
  const std::string host;
  int port;

  ImgHttp(const std::string &host, int port, const std::string &dbPath)
      : host(host), port(port), db(nullptr, sqlite3_close) {
    sqlite3 *tempDb;
    int rc = sqlite3_open(dbPath.c_str(), &tempDb);
    db.reset(tempDb);
    if (rc != SQLITE_OK) {
      std::cerr << "Cannot open database: " << sqlite3_errmsg(db.get());
    }
    createTable();
  }

public:
  ImgHttp(const ImgHttp &) = delete;
  ImgHttp &operator=(const ImgHttp &) = delete;

  static ImgHttp &server(const std::string &host = "0.0.0.0", int port = 80,
                         const std::string &dbPath = "newtest.db") {
    static ImgHttp instance(host, port, dbPath);
    return instance;
  }

  void run() {
    drogon::app().addListener("0.0.0.0", 80);
    drogon::app().run();
  }

  void createTable() {
    char *err_msg = 0;
    std::string sql =
        "CREATE TABLE IF NOT EXISTS images(link TEXT PRIMARY KEY, data BLOB);";
    int rc = sqlite3_exec(db.get(), sql.c_str(), 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to create table: " << err_msg;
      sqlite3_free(err_msg);
    }
  }

  bool checkImage(const std::string &imageLink) {
    std::string sql = "SELECT COUNT(*) FROM images WHERE link = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db.get(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db.get());
      return false;
    }

    rc = sqlite3_bind_text(stmt, 1, imageLink.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to bind value: " << sqlite3_errmsg(db.get());
      sqlite3_finalize(stmt);
      return false;
    }

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      int count = sqlite3_column_int(stmt, 0);
      exists = (count > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
  }

  void addImage(const std::string &imageLink, const std::string &imageData) {
    std::string sql = "INSERT INTO images (link, data) VALUES (?, ?)";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db.get(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db.get());
      return;
    }

    rc = sqlite3_bind_text(stmt, 1, imageLink.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to bind value: " << sqlite3_errmsg(db.get());
      sqlite3_finalize(stmt);
      return;
    }

    rc = sqlite3_bind_blob(stmt, 2, imageData.data(), imageData.size(),
                           SQLITE_STATIC);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to bind value: " << sqlite3_errmsg(db.get());
      sqlite3_finalize(stmt);
      return;
    }

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
      std::cerr << "Failed to execute statement: " << sqlite3_errmsg(db.get());
    }

    sqlite3_finalize(stmt);
  }

  std::string getImage(const std::string &imageLink) {
    std::string sql = "SELECT data FROM images WHERE link = ?";
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db.get(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db.get());
      return "";
    }

    rc = sqlite3_bind_text(stmt, 1, imageLink.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) {
      std::cerr << "Failed to bind value: " << sqlite3_errmsg(db.get());
      sqlite3_finalize(stmt);
      return "";
    }

    std::string imageData;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const void *data = sqlite3_column_blob(stmt, 0);
      int dataSize = sqlite3_column_bytes(stmt, 0);
      imageData = std::string(static_cast<const char *>(data), dataSize);
    }

    sqlite3_finalize(stmt);
    return imageData;
  }
};

using namespace drogon;

class ImageProcessor : public drogon::HttpController<ImageProcessor> {

private:
  ImgHttp &cache;

public:
  ImageProcessor() : cache(ImgHttp::server()) {}

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(ImageProcessor::sayHello, "/", Get);
  ADD_METHOD_TO(ImageProcessor::processImage, "/", Post);
  ADD_METHOD_TO(ImageProcessor::resizeAndReturnImage, "/resize", Post);
  METHOD_LIST_END

  void sayHello(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback) {
    auto response = HttpResponse::newHttpResponse();
    response->setBody("Welcome to Kalz' High Performance Web Server");
    callback(response);
  }

  static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                              std::ostringstream *outStream) {
    size_t totalSize = size * nmemb;
    outStream->write(static_cast<const char *>(contents), totalSize);
    return totalSize;
  }

  void resizeAndReturnImage(
      const HttpRequestPtr &req,
      std::function<void(const HttpResponsePtr &)> &&callback) {
    if (req->method() == HttpMethod::Post) {
      const auto &body = req->getBody();
      std::string bodyString(body.data(), body.length());
      Json::CharReaderBuilder builder;
      Json::Value json;
      JSONCPP_STRING err;
      std::istringstream bodyStream(bodyString);
      if (Json::parseFromStream(builder, bodyStream, &json, &err)) {
        const std::string imageLink = json["imageLink"].asString();
        const int imageWidth = json["width"].asInt();
        const int imageHeight = json["height"].asInt();
        const bool retainFormat = json["retainFormat"].asBool();

        LOG_DEBUG << "Passed Image Dimensions: " << imageWidth << "x"
                  << imageHeight;

        std::string imageContents;

        // Check if the image is present in the cache
        if (cache.checkImage(imageLink)) {
          // Retrieve the image contents from the cache
          imageContents = cache.getImage(imageLink);
          LOG_DEBUG << "Image retrieved from cache";
        } else {
          std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
              curl_easy_init(), curl_easy_cleanup);

          if (curl) {
            std::ostringstream outStream;
            curl_easy_setopt(curl.get(), CURLOPT_URL, imageLink.c_str());
            curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &outStream);

            // Perform the image download
            CURLcode res = curl_easy_perform(curl.get());
            if (res != CURLE_OK) {
              LOG_ERROR << "Failed to download image: "
                        << curl_easy_strerror(res);
              auto errorResponse = HttpResponse::newHttpResponse();
              errorResponse->setStatusCode(
                  HttpStatusCode::k500InternalServerError);
              errorResponse->setBody("Failed to download image");
              callback(errorResponse);
              return;
            }

            LOG_DEBUG << "Download Success ";
            LOG_DEBUG << "Image Downloaded";

            imageContents = outStream.str();

            // Add the image to the cache
            cache.addImage(imageLink, imageContents);
          } else {
            LOG_ERROR << "Failed to initialize CURL";
            auto errorResponse = HttpResponse::newHttpResponse();
            errorResponse->setStatusCode(
                HttpStatusCode::k500InternalServerError);
            errorResponse->setBody("Failed to initialize CURL");
            callback(errorResponse);
            return;
          }
        }

        // Load image data into OpenCV
        std::vector<char> imageData(imageContents.begin(), imageContents.end());
        size_t dataSize = imageData.size();
        LOG_DEBUG << "Image Size: " << dataSize;

        cv::Mat img =
            cv::imdecode(cv::Mat(1, dataSize, CV_8UC1, imageData.data()),
                         cv::IMREAD_UNCHANGED);

        // Check if image data is valid
        if (img.empty()) {
          LOG_ERROR << "Failed to decode image";
          auto errorResponse = HttpResponse::newHttpResponse();
          errorResponse->setStatusCode(HttpStatusCode::k500InternalServerError);
          errorResponse->setBody("Failed to decode image");
          callback(errorResponse);
          return;
        }

        // Log the original dimensions
        int originalWidth = img.cols;
        int originalHeight = img.rows;
        LOG_DEBUG << "Original Image Dimensions: " << originalWidth << "x"
                  << originalHeight;

        // Log Aspect Ratio
        double originalAspectRatio = (double)originalWidth / originalHeight;
        LOG_DEBUG << "Original Image Aspect Ratio: " << originalAspectRatio;

        // Calculate the target size while maintaining aspect ratio
        int targetWidth = imageWidth;
        int targetHeight = imageHeight;

        if (retainFormat) {
          // Calculate the new size based on the specified dimensions while
          // maintaining aspect ratio
          if (originalAspectRatio > 1) {
            targetHeight = static_cast<int>(targetWidth / originalAspectRatio);
          } else {
            targetWidth = static_cast<int>(targetHeight * originalAspectRatio);
          }

          // Ensure that the target dimensions are not larger than the original
          // image dimensions
          targetWidth = std::min(targetWidth, originalWidth);
          targetHeight = std::min(targetHeight, originalHeight);

          // Resize the image using OpenCV and preserve the aspect ratio
          cv::Mat resizedImg;
          cv::resize(img, resizedImg, cv::Size(targetWidth, targetHeight), 0, 0,
                     cv::INTER_AREA);

          // Create a transparent or white border based on image format
          cv::Scalar borderColor;

          if (img.channels() == 4) {
            // If the image has an alpha channel, create a transparent border
            borderColor = cv::Scalar(0, 0, 0, 0);
          } else {
            // If the image doesn't have an alpha channel, create a white border
            borderColor = cv::Scalar(255, 255, 255);
          }

          // Calculate the border sizes to maintain the target dimensions
          int topBorder = (imageHeight - targetHeight) / 2;
          int bottomBorder = imageHeight - targetHeight - topBorder;
          int leftBorder = (imageWidth - targetWidth) / 2;
          int rightBorder = imageWidth - targetWidth - leftBorder;

          // Ensure that the border sizes are non-negative
          topBorder = std::max(topBorder, 0);
          bottomBorder = std::max(bottomBorder, 0);
          leftBorder = std::max(leftBorder, 0);
          rightBorder = std::max(rightBorder, 0);

          // Add borders to the resized image while preserving the aspect ratio
          cv::copyMakeBorder(resizedImg, resizedImg, topBorder, bottomBorder,
                             leftBorder, rightBorder, cv::BORDER_CONSTANT,
                             borderColor);

          // Encode the resized image
          std::vector<uchar> buffer;
          cv::imencode(".png", resizedImg, buffer);

          // Create a string from the buffer to send in response
          std::string resizedImageStr(buffer.begin(), buffer.end());

          // Create response
          auto response = HttpResponse::newHttpResponse();
          response->setBody(resizedImageStr);
          callback(response);

          // Log the resized dimensions
          int resizedWidth = resizedImg.cols;
          int resizedHeight = resizedImg.rows;
          LOG_DEBUG << "Resized Image Dimensions: " << resizedWidth << "x"
                    << resizedHeight;
        } else {
          // Convert the image to PNG format
          cv::Mat pngImg;
          cv::cvtColor(img, pngImg, cv::COLOR_BGR2BGRA);

          // Calculate the new size based on the specified dimensions while
          // maintaining aspect ratio
          if (originalAspectRatio > 1) {
            targetHeight = static_cast<int>(targetWidth / originalAspectRatio);
          } else {
            targetWidth = static_cast<int>(targetHeight * originalAspectRatio);
          }

          // Ensure that the target dimensions are not larger than the original
          // image dimensions
          targetWidth = std::min(targetWidth, originalWidth);
          targetHeight = std::min(targetHeight, originalHeight);

          // Resize the image using OpenCV and preserve the aspect ratio
          cv::Mat resizedImg;
          cv::resize(pngImg, resizedImg, cv::Size(targetWidth, targetHeight), 0,
                     0, cv::INTER_LANCZOS4);

          // Create a transparent border based on the target dimensions
          cv::Scalar borderColor(0, 0, 0, 0);

          // Calculate the border sizes to maintain the target dimensions
          int topBorder = (imageHeight - targetHeight) / 2;
          int bottomBorder = imageHeight - targetHeight - topBorder;
          int leftBorder = (imageWidth - targetWidth) / 2;
          int rightBorder = imageWidth - targetWidth - leftBorder;

          // Ensure that the border sizes are non-negative
          topBorder = std::max(topBorder, 0);
          bottomBorder = std::max(bottomBorder, 0);
          leftBorder = std::max(leftBorder, 0);
          rightBorder = std::max(rightBorder, 0);

          // Add borders to the resized image while preserving the aspect ratio
          cv::copyMakeBorder(resizedImg, resizedImg, topBorder, bottomBorder,
                             leftBorder, rightBorder, cv::BORDER_CONSTANT,
                             borderColor);

          // Encode the resized image as PNG
          std::vector<uchar> buffer;
          cv::imencode(".png", resizedImg, buffer);

          // Create a string from the buffer to send in response
          std::string resizedImageStr(buffer.begin(), buffer.end());

          // Create response
          auto response = HttpResponse::newHttpResponse();
          response->setBody(resizedImageStr);
          callback(response);

          // Log the resized dimensions
          int resizedWidth = resizedImg.cols;
          int resizedHeight = resizedImg.rows;
          LOG_DEBUG << "Resized Image Dimensions: " << resizedWidth << "x"
                    << resizedHeight;
        }
      } else {
        LOG_ERROR << "Invalid JSON Format";
        auto errorResponse = HttpResponse::newHttpResponse();
        errorResponse->setStatusCode(HttpStatusCode::k400BadRequest);
        errorResponse->setBody("Invalid JSON Format");
        callback(errorResponse);
      }
    } else {
      LOG_ERROR << "Method Not Allowed";
      auto errorResponse = HttpResponse::newHttpResponse();
      errorResponse->setStatusCode(HttpStatusCode::k405MethodNotAllowed);
      errorResponse->setBody("Method Not Allowed");
      callback(errorResponse);
    }
  }

  void processImage(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback) {
    if (req->method() == HttpMethod::Post) {
      const auto &body = req->getBody();
      std::string bodyString(body.data(), body.length());
      Json::CharReaderBuilder builder;
      Json::Value json;
      JSONCPP_STRING err;
      std::istringstream bodyStream(bodyString);
      if (Json::parseFromStream(builder, bodyStream, &json, &err)) {
        const std::string imageLink = json["imageLink"].asString();
        const std::string imageSize = json["size"].asString();
        Json::Value jsonResponse;
        jsonResponse["imageLink"] = imageLink;
        jsonResponse["size"] = imageSize;
        auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
        callback(response);
      } else {
        LOG_DEBUG << "Invalid JSON Format";
        auto errorResponse = HttpResponse::newHttpResponse();
        errorResponse->setStatusCode(HttpStatusCode::k400BadRequest);
        errorResponse->setBody("Invalid JSON Format");
        callback(errorResponse);
      }
    } else {
      LOG_DEBUG << "Method Not Allowed";
      auto errorResponse = HttpResponse::newHttpResponse();
      errorResponse->setStatusCode(HttpStatusCode::k405MethodNotAllowed);
      errorResponse->setBody("Method Not Allowed");
      callback(errorResponse);
    }
  }

  void downloadImage(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback) {
    if (req->method() == HttpMethod::Post) {
      const auto &body = req->getBody();
      std::string bodyString(body.data(), body.length());
      Json::CharReaderBuilder builder;
      Json::Value json;
      JSONCPP_STRING err;
      std::istringstream bodyStream(bodyString);
      if (Json::parseFromStream(builder, bodyStream, &json, &err)) {
        const std::string imageLink = json["imageLink"].asString();
        const std::string imageSize = json["size"].asString();

        // Extract the image name from the URL
        std::string imageName =
            imageLink.substr(imageLink.find_last_of("/") + 1);

        // Check if the image is present in the cache
        if (cache.checkImage(imageLink)) {
          Json::Value jsonResponse;
          jsonResponse["message"] = "Image present in cache";
          auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
          callback(response);
          return;
        }

        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(
            curl_easy_init(), curl_easy_cleanup);
        if (curl) {
          std::ostringstream outStream;
          curl_easy_setopt(curl.get(), CURLOPT_URL, imageLink.c_str());
          curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
          curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &outStream);
          curl_easy_perform(curl.get());

          std::string imageContents = outStream.str();

          //   Add the image to the cache
          cache.addImage(imageLink, "imageContents");
        }

        Json::Value jsonResponse;
        jsonResponse["message"] = "Image downloaded successfully";
        auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
        callback(response);
      } else {
        LOG_DEBUG << "Invalid JSON Format";
        auto errorResponse = HttpResponse::newHttpResponse();
        errorResponse->setStatusCode(HttpStatusCode::k400BadRequest);
        errorResponse->setBody("Invalid JSON Format");
        callback(errorResponse);
      }
    } else {
      LOG_DEBUG << "Method Not Allowed";
      auto errorResponse = HttpResponse::newHttpResponse();
      errorResponse->setStatusCode(HttpStatusCode::k405MethodNotAllowed);
      errorResponse->setBody("Method Not Allowed");
      callback(errorResponse);
    }
  }
};

#endif
