# C++ OpenCV Drogon Server 


 _High Performance Multi Threaded C++ Image Processing Web Server_

</br>

## Features 

</br>


-  Resize Images by simply providing a `Link`, `Width`, and `Height`
-  Inbuilt `Caching` and `Multithreading` for `High Performance`
-  Automatically Detect Image Format and Perform Conversions 
-  Adds Transparent Padding to Maintain Original Aspect Ratio
-  Download Images
-  Convert Images between `JPEG` , `WEBP` , `PNG`

</br>


- `OpenCV` Image Resizing Algorithms :
  - `INTER_NEAREST` - a nearest-neighbor interpolation
  - `INTER_LINEAR` - a bilinear interpolation (used by default)
  - `INTER_AREA` - resampling using pixel area relation. It may be a preferred method for image decimation, as it gives moire’-free results. But when the image is zoomed, it is similar to the `INTER_NEAREST` method.
  - `INTER_CUBIC` - a bicubic interpolation over 4x4 pixel neighborhood
  - `INTER_LANCZOS4` - a Lanczos interpolation over 8x8 pixel neighborhood

</br>

- Fastest Algorithm - `INTER_NEAREST`
- Highest Quality Algorithm - `INTER_AREA` || `INTER_LANCZOS4`

## Usage and Performance  

</br>


```bash
# Running the Image Server
docker run --rm --name img -p 80:80 imghttp

# Viewing the Images
docker exec imghttp2 ls /app/images
```
- Endpoints 

```bash
# Commands 

# Hello World
curl -X GET http://localhost

# POST Return Link
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://example.com/image.jpg", "size": "500x400" }' http://localhost

# Download Image 
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://example.com/image.jpg", "size": "500x400" }' http://localhost:80/download

# Resize Image 
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://ex.com/image.jpg", "width": 75, "height": 150, "retainFormat": true }' http://localhost:80/resize --output resizedImage5.jpeg

# Sample Commands


# Resize an Image (JPEG)
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://images.pexels.com/photos/5230612/pexels-photo-5230612.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=1", "width": 75, "height": 150, "retainFormat": true }' http://localhost:80/resize --output resizedImage5.jpeg

# Resize an Image (WebP)
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://cdn.mos.cms.futurecdn.net/GDy6nDyEtUkJbEvEpqDgoB-1600-80.jpg.webp", "width": 100, "height": 150, "retainFormat": false }' http://localhost:80/resize --output resizedImage3.png

# Resize Cat Image (WebP)
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://static01.nyt.com/images/2021/09/14/science/07CAT-STRIPES/07CAT-STRIPES-superJumbo.jpg?quality=75&auto=webp", "width": 100, "height": 150, "retainFormat": false }' http://localhost:80/resize --output resizedImage3.png


# Download an Image 
curl -X POST -H "Content-Type: application/json" -d '{ "imageLink": "https://images.pexels.com/photos/5230612/pexels-photo-5230612.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=1", "size": "500x400" }' http://localhost/download


docker exec img ls /app/images

```

## Performance 

- Create `request.lua` file

```lua
wrk.method = "POST"
wrk.headers["Content-Type"] = "application/json"
wrk.body = '{ "imageLink": "https://images.pexels.com/photos/5230612/pexels-photo-5230612.jpeg?auto=compress&cs=tinysrgb&w=1260&h=750&dpr=1", "width": 100, "height": 150, "format": "jpg" }'

function setup(thread)
   thread:set("threadid", thread.id)
end

function init(args)
   requests = 10  -- Number of concurrent requests
   thread_id = 0
end

function request()
   thread_id = thread_id + 1
   local id = thread_id % requests
   local path = "/resize?id=" .. id
   return wrk.format("POST", path, nil, wrk.body)
end
```

- Run `Performance Test`

```bash
# Measuring Performance using wrk 
sudo apt-get install wrk

# Small Load
wrk -c 10 -t 10 -d 10s -s request.lua http://localhost:80

# Large Load 
wrk -c 100 -t 1000 -d 30s -s request.lua http://localhost:80


# Monitoring CPU and Memory Usage 

sudo apt-get install docker-stats
docker stats <container_id>

# Increasing number of Open Files that can be created 
ulimit -n # Shows file limit 
ulimit -n 10000 # Sets file limit to 10000

```



- `C++` Sample Code included as a single `.cpp` file in case you want to run a Drogon server yourself 

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
# Base image
FROM ubuntu:20.04

# Set environment variables to noninteractive and set timezone
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles

# Install necessary packages
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
    libstdc++-9-dev \
    libcurl4-openssl-dev \
    curl \
    libpq-dev \
    libopencv-dev \
    libspdlog-dev \
    sqlite3 \
    libsqlite3-dev


# Clone and build Trantor library
RUN git clone https://github.com/an-tao/trantor.git /trantor
WORKDIR /trantor
RUN mkdir build
WORKDIR /trantor/build
RUN cmake ..
RUN make && make install

# Clone and build Drogon framework
WORKDIR /
RUN git clone https://github.com/an-tao/drogon.git /drogon
WORKDIR /drogon
RUN git submodule update --init
RUN mkdir build
WORKDIR /drogon/build
RUN cmake ..
RUN make && make install

# Set workdir
WORKDIR /app

# Copy the current folder which contains C++ source code to the Docker image under /app
COPY ./c++ /app/c++

# Specify the build command
RUN g++ -std=c++17 -o myapp c++/main.cpp c++/Controller.cpp c++/ImageCache.cpp -l drogon -l trantor -l jsoncpp -l uuid -l ssl -l crypto -l boost_system -l pthread -ldl -lz -l curl -l opencv_core -l opencv_imgproc -l opencv_imgcodecs -l spdlog -lsqlite3 -I /usr/include/jsoncpp -I /usr/include/opencv4 -I /usr/local/include -I /usr/local/include/spdlog -L /usr/local/lib

# Expose port 80 to the outside
EXPOSE 80

# Command to run the executable
CMD ["./myapp"]
```