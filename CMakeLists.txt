cmake_minimum_required(VERSION 3.14)
project(ImgHTTP)

# change standard as required
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

list(APPEND CMAKE_PREFIX_PATH "drogon_ctl")
find_package(Drogon CONFIG REQUIRED)
find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIR})

list(APPEND CMAKE_PREFIX_PATH "opencv4")
find_package(OpenCV CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE  
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


add_executable(
  simple
  src/simple.cc
)

target_link_libraries(
  simple
  Drogon::Drogon
  GTest::gtest_main
  ${CURL_LIBRARIES}
  ${OpenCV_LIBS}
)

enable_testing()

add_executable(
  imghttp_test 
  tests/imghttp_test.cc
)

target_link_libraries(
  imghttp_test
  GTest::gtest_main
  Drogon::Drogon
  ${CURL_LIBRARIES}
  ${OpenCV_LIBS}
)


include(GoogleTest)
gtest_discover_tests(imghttp_test)

# cmake ..
# make