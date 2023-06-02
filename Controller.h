#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <drogon/HttpController.h>

using namespace drogon;

class HelloWorldController : public drogon::HttpController<HelloWorldController>
{
public:
  METHOD_LIST_BEGIN
  ADD_METHOD_TO(HelloWorldController::sayHello, "/", Get);
  ADD_METHOD_TO(HelloWorldController::processImage, "/", Post);
  METHOD_LIST_END

  void sayHello(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
  void processImage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback);
};

#endif
