#include <iostream>

#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>

const int DOC_WIDTH = 80;

using namespace cgen::target;

auto main() -> int {

  auto expr = Fab::test_file({{"command", {"&nwocg.Add3", 0}},
                              {"feedback", {"&nwocg.feedback", 1}},
                              {"setpoint", {"&nwocg.setpoint", 1}}},
                             {"setpoint", "feedback", "Add1", "I_gain", "Ts",
                              "P_gain", "UnitDelay1", "Add2", "Add3"});
  auto writer = CodeWriter(std::cout, DOC_WIDTH);
  writer.write(expr);

  return 0;
}
