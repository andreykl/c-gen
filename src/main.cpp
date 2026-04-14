#include <iostream>

#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>

const int DOC_WIDTH = 80;

using namespace cgen::target;

auto main() -> int {

  vector<InitField> init_fields = {Fab::init_field("UnitDelay1", 0)};
  vector<Operation> step_fields = {Fab::op("Add1", "setpoint", "-", "feedback"),
                                   Fab::op("I_gain", "Add1", "*", 2),
                                   Fab::op("Ts", "I_gain", "*", 0.01),
                                   Fab::op("P_gain", "Add1", "*", 3),
                                   Fab::op("Add2", "Ts", "+", "UnitDelay1"),
                                   Fab::op("Add3", "P_gain", "+", "Add2"),
                                   Fab::op("UnitDelay1", "Add2")};

  auto expr = Fab::test_file({{"command", {"&nwocg.Add3", 0}},
                              {"feedback", {"&nwocg.feedback", 1}},
                              {"setpoint", {"&nwocg.setpoint", 1}}},
                             {"setpoint", "feedback", "Add1", "I_gain", "Ts",
                              "P_gain", "UnitDelay1", "Add2", "Add3"},
                             init_fields, step_fields);
  auto writer = CodeWriter(std::cout, DOC_WIDTH);
  writer.write(expr);

  return 0;
}
