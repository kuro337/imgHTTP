# Base image
FROM ubuntu:20.04

# Set environment variables to noninteractive and set timezone
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=America/Los_Angeles

# Install necessary packages
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libgtest-dev \
    libjsoncpp-dev \
    libopencv-dev

# Set workdir
WORKDIR /app/tests

# Copy the current folder which contains the test files to the Docker image under /app/tests
COPY . .

# Build the tests
RUN g++ -std=c++17 -o mytests tests.cpp ../c++/Controller.cpp ../c++/ImageCache.cpp -l gtest -l pthread -I /usr/local/include -I /usr/include/jsoncpp -I /usr/include/opencv4

# Run the tests
CMD ["./mytests"]
