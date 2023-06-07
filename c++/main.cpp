#include "Controller.h"
#include "ImageCache.h"
#include <drogon/drogon.h>

int main() {
  // Create SQLite database and table
  ImageCache cache("image_database.db");
  cache.createTable();

  // Run Drogon application
  drogon::app().addListener("0.0.0.0", 80);
  drogon::app().run();

  return 0;
}
