#include "server/imghttp.h"

#include <drogon/drogon.h>

int main() {

  ImgHttp &img_server = ImgHttp::server("0.0.0.0", 80, "img_cache.db");

  img_server.run();

  return 0;
}
