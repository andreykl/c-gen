#include <c-gen/source/graph.hpp>
#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>
#include <iostream>

const int DOC_WIDTH = 80;

using namespace cgen::target;
using namespace cgen::source;

auto main() -> int {
  Graph g;
  g.add_inport("setpoint", "16");
  g.add_inport("feedback", "18");
  g.add_sum("Add1", "17", {"2", "1"}, {1, -1});
  g.add_sum("Add2", "22", {"2", "1"}, {});
  g.add_sum("Add3", "23", {"2", "1"}, {});
  g.add_gain("I_gain", "25", 2);
  g.add_gain("P_gain", "19", 3);
  g.add_gain("Ts", "26", 0.01);
  g.add_unit_delay("Unit Delay1", "21", -1);
  g.add_outport("command", "20");

  g.add_line("16", "17", "1");
  g.add_line("18", "17", "2");
  g.add_line("17", "25", "1");
  g.add_line("17", "19", "1");
  g.add_line("21", "22", "2");
  g.add_line("22", "21", "1");
  g.add_line("22", "23", "2");
  g.add_line("19", "23", "1");
  g.add_line("23", "20", "1");
  g.add_line("25", "26", "1");
  g.add_line("26", "22", "1");

  g.transform(CodeWriter(std::cout, DOC_WIDTH));

  return 0;
}
