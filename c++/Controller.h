#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "ImageCache.h"
#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>

using namespace drogon;

class HelloWorldController
    : public drogon::HttpController<HelloWorldController> {
public:
  HelloWorldController();

  METHOD_LIST_BEGIN
  ADD_METHOD_TO(HelloWorldController::sayHello, "/", Get);
  ADD_METHOD_TO(HelloWorldController::processImage, "/", Post);
  ADD_METHOD_TO(HelloWorldController::downloadImage, "/download", Post);
  ADD_METHOD_TO(HelloWorldController::resizeAndReturnImage, "/resize", Post);
  METHOD_LIST_END

  void sayHello(const HttpRequestPtr &req,
                std::function<void(const HttpResponsePtr &)> &&callback);
  void processImage(const HttpRequestPtr &req,
                    std::function<void(const HttpResponsePtr &)> &&callback);
  void downloadImage(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
  void
  resizeAndReturnImage(const HttpRequestPtr &req,
                       std::function<void(const HttpResponsePtr &)> &&callback);

private:
  ImageCache cache;
};

#endif
