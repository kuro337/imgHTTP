#include <drogon/drogon.h>
#include "Controller.h"

int main()
{
  drogon::app().addListener("0.0.0.0", 80); // Listen on port 80
  drogon::app().run();

  return 0;
}
