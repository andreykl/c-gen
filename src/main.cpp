#include <iostream>
#include <prettyprint.h>

const int DOC_WIDTH = 80;

auto main() -> int {
  auto doc = pp::group(pp::text("void") + pp::softbreak() + pp::text("main()"));
  auto settings = std::cout << pp::set_width(DOC_WIDTH);
  settings << doc << std::endl;

  return 0;
}
