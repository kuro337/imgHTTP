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
