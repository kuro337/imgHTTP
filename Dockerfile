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
    libpq-dev

# Clone and build Trantor library
RUN git clone https://github.com/an-tao/trantor.git
WORKDIR /trantor
RUN mkdir build
WORKDIR /trantor/build
RUN cmake ..
RUN make && make install

# Clone and build Drogon framework
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
RUN g++ -std=c++14 -o myapp main.cpp Controller.cpp -l drogon -l trantor -l jsoncpp -l uuid -l ssl -l crypto -l boost_system -l pthread -ldl -lz -I /usr/include/jsoncpp

# Expose port 80 to the outside
EXPOSE 80

# Command to run the executable
CMD ["./myapp"]


