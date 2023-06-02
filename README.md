# C++ OpenCV Drogon Server 


- High Performance C++ Web Server

- `C++` Code Below included as a single `.cpp` file in case you want to run it yourself 

```cpp

#include <drogon/drogon.h>
#include <json/json.h>

using namespace drogon;

class HelloWorldController : public drogon::HttpController<HelloWorldController>
{
public:
  METHOD_LIST_BEGIN
  // Handler for GET request "/"
  ADD_METHOD_TO(HelloWorldController::sayHello, "/", Get);
  // Handler for POST request "/"
  ADD_METHOD_TO(HelloWorldController::processImage, "/", Post);
  METHOD_LIST_END

  void sayHello(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
  {
    auto response = HttpResponse::newHttpResponse();
    response->setBody("Welcome to Kalz' High Performance Web Server");
    callback(response);
  }

  void processImage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback)
  {
    if (req->method() == HttpMethod::Post)
    {
      const auto &body = req->getBody();
      std::string bodyString(body.data(), body.length()); // Convert boost::string_view to std::string
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
};

int main()
{
  app().addListener("0.0.0.0", 80); // Listen on port 80
  app().run();
  return 0;
}


```


- `Dockerfile` to build 

```dockerfile
FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles

# Packages
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    libssl-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    git \
    libboost-all-dev \
    libpthread-stubs0-dev \
    libjsoncpp-dev \
    uuid-dev \
    zlib1g-dev \
    openssl \
    libssl-dev \
    libmysqlclient-dev \
    libpq-dev

# Clone and Build Trantor 
RUN git clone https://github.com/an-tao/trantor.git
WORKDIR /trantor
RUN mkdir build
WORKDIR /trantor/build
RUN cmake ..
RUN make && make install

# Drogon C++ Web Server
WORKDIR /
RUN git clone https://github.com/an-tao/drogon.git
WORKDIR /drogon
RUN git submodule update --init
RUN mkdir build
WORKDIR /drogon/build
RUN cmake ..
RUN make && make install

# Set workdir
WORKDIR /app

# Copy the current folder which contains C++ source code to the Docker image under /app
COPY . /app

# Specify the build command
RUN g++ -std=c++14 -o myapp main.cpp -l drogon -l trantor -l jsoncpp -l uuid -l ssl -l crypto -l boost_system -l pthread -ldl -lz -I /usr/include/jsoncpp

# Expose port 80 to the outside
EXPOSE 80

# Command to run the executable
CMD ["./myapp"]

```