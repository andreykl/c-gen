#pragma once
#include <c-gen/source/node.hpp>
#include <c-gen/target/api/fab.hpp>
#include <c-gen/target/api/iwriter.hpp>
#include <c-gen/target/api/structs.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace cgen::target::api;

namespace cgen::source {
class Graph {
public:
  auto add_inport(string name, string SID) -> void;
  auto add_sum(string name, string SID, const vector<string> &portnames,
               const vector<int> &portsigns) -> void;
  auto add_gain(string name, string SID, double gain) -> void;
  auto add_unit_delay(string name, string SID, int sample_time) -> void;
  auto add_outport(string name, string SID) -> void;
  auto add_line(string fromSID, string toSID,
                size_t to_port_ind /* index of port in ins */) -> void;
  auto transform(const IWriter &writer) const -> void;

private:
  unordered_map<string, Node> nodes;
  string outportSID;
  unordered_set<string> inportSIDs;

  auto traverse(const Node &node, vector<Operation> &steps,
                vector<Operation> &delays, vector<Port> &ports,
                vector<string> &struct_fields, vector<InitField> &init_fields,
                unordered_set<string> &visited,
                unordered_set<string> &in_progress) const -> void;

  auto get_add_line_err1(string fromSID, string toSID, int to_port_ind,
                         string SID) const -> string;
  auto get_add_line_err2(string fromSID, string toSID,
                         int to_port_ind) const -> string;
  auto add_node(Node &&node) -> void;
  auto node_name_cl(string SID) const -> string;
  auto clean(string name) const -> string;
};
} // namespace cgen::source
