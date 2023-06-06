#include "Controller.h"
#include "ImageCache.h"
#include <drogon/drogon.h>

int main() {
  
  ImageCache cache("image_database.db");
  cache.createTable();

  drogon::app().addListener("0.0.0.0", 80);
  drogon::app().run();

  return 0;
}
