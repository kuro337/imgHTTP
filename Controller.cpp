#include "Controller.h"
#include <drogon/HttpResponse.h>
#include <json/json.h>

void HelloWorldController::sayHello(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
  auto response = HttpResponse::newHttpResponse();
  response->setBody("Welcome to Kalz' High Performance Web Server");
  callback(response);
}

void HelloWorldController::processImage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
{
  if (req->method() == HttpMethod::Post)
  {
    const auto &body = req->getBody();
    std::string bodyString(body.data(), body.length());
    Json::CharReaderBuilder builder;
    Json::Value json;
    JSONCPP_STRING err;
    std::istringstream bodyStream(bodyString);
    if (Json::parseFromStream(builder, bodyStream, &json, &err))
    {
      const std::string imageLink = json["imageLink"].asString();
      const int imageSize = json["size"].asInt();
      Json::Value jsonResponse;
      jsonResponse["imageLink"] = imageLink;
      auto response = HttpResponse::newHttpJsonResponse(jsonResponse);
      callback(response);
    }
    else
    {
      auto errorResponse = HttpResponse::newHttpResponse();
      errorResponse->setStatusCode(HttpStatusCode::k400BadRequest);
      errorResponse->setBody("Invalid JSON Format");
      callback(errorResponse);
    }
  }
  else
  {
    auto errorResponse = HttpResponse::newHttpResponse();
    errorResponse->setStatusCode(HttpStatusCode::k405MethodNotAllowed);
    errorResponse->setBody("Method Not Allowed");
    callback(errorResponse);
  }
}
