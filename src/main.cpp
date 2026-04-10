#include <iostream>

#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>

const int DOC_WIDTH = 80;

using namespace cgen::target;

auto main() -> int {
  auto expr = Fab::extPorts();
  auto writer = CodeWriter(std::cout, DOC_WIDTH);
  writer.write(expr);

  return 0;
}
