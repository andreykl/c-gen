#include <c-gen/target/ast.hpp>
#include <c-gen/target/iwriter.hpp>
#include <format>
#include <ranges>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;

namespace cgen::source {
auto node_err_info(string type, string name, string SID) -> string {
  return format("name = '{}', type = '{}', SID = '{}'", std::move(name),
                std::move(type), std::move(SID));
}

class Graph {
private:
  class Node {
    friend class Graph;

  public:
    auto inline node_err_info() const -> string {
      return cgen::source::node_err_info(to_string(type), name, SID);
    }

  private:
    enum class Type { Inport, Outport, Sum, Gain, UnitDelay };

    struct Inport {
      string fromSID;
      int sign = 1; // +1/-1
    };

    Type type;
    string name;
    string SID;
    vector<Inport> ins;
    unordered_map<string, size_t> portmapping;

    union {
      double gain;
      int sample_time;
    };

    static auto to_string(Type t) -> string {
      using enum Type;
      switch (t) {
      case Inport:
        return "Inport";
      case Outport:
        return "Outport";
      case Sum:
        return "Sum";
      case Gain:
        return "Gain";
      case UnitDelay:
        return "UnitDelay";
      }
      return "Unknown";
    }

    explicit Node(Type type, string name, string SID)
        : type(type), name(std::move(name)), SID(std::move(SID)),
          sample_time(0){};

    explicit Node(Type type, string name, string SID, double gain)
        : type(type), name(std::move(name)), SID(std::move(SID)), gain(gain){};

    explicit Node(Type type, string name, string SID, int sample_time)
        : type(type), name(std::move(name)), SID(std::move(SID)),
          sample_time(sample_time){};

    auto add_inport(string name, int sign) -> void {
      if (portmapping.contains(name)) {
        throw std::runtime_error(
            format("Problem during building a graph: portmapping "
                   "already contains port with name '{}'. Interrupting.",
                   std::move(name)));
      }
      portmapping.insert({std::move(name), ins.size()});
      ins.emplace_back("", sign);
    }
  };

public:
  inline auto add_inport(string name, string SID) -> void {
    add_node(Node{Node::Type::Inport, std::move(name), SID});
    inportSIDs.insert(std::move(SID));
  };

  auto add_sum(string name, string SID, const vector<string> &portnames,
               const vector<int> &portsigns) -> void {

    vector<int> fallback;
    if (portsigns.empty())
      fallback.assign(portnames.size(), 1);

    auto &signs = portsigns.empty() ? fallback : portsigns;

    if (signs.size() != portnames.size())
      throw runtime_error(format(
          "Problem during building a graph: portsigns are not empty and "
          "portsigns.size != portname.size. Node info: {}. Interrupting.",
          node_err_info(Node::to_string(Node::Type::Sum), std::move(name),
                        std::move(SID))));

    if (portnames.size() != 2) {
      throw runtime_error(
          format("Problem during building a graph: we must have 2 ports for "
                 "Sum type node, but we have {}. Node info: {}. Interrupting.",
                 portnames.size(),
                 node_err_info(Node::to_string(Node::Type::Sum),
                               std::move(name), std::move(SID))));
    }

    auto node = Node{Node::Type::Sum, std::move(name), std::move(SID)};

    for (auto &&[name, sign] : views::zip(portnames, signs)) {
      node.add_inport(name, sign);
    }

    add_node(std::move(node));
  };

  inline auto add_gain(string name, string SID, double gain) -> void {
    add_node(Node{Node::Type::Gain, std::move(name), std::move(SID), gain});
  }

  inline auto add_unit_delay(string name, string SID, int sample_time) -> void {
    add_node(Node{Node::Type::UnitDelay, std::move(name), std::move(SID),
                  sample_time});
  }

  inline auto add_outport(string name, string SID) -> void {
    add_node(Node{Node::Type::Outport, std::move(name), SID});
    outportSID = std::move(SID);
  }

  auto add_line(string fromSID, string toSID, string to_port) -> void {
    if (!nodes.contains(toSID)) {
      throw std::runtime_error(
          get_add_line_err1(std::move(fromSID), std::move(toSID),
                            std::move(to_port), std::move(toSID)));
    }

    if (!nodes.contains(fromSID)) {
      throw std::runtime_error(
          get_add_line_err1(std::move(fromSID), std::move(toSID),
                            std::move(to_port), std::move(fromSID)));
    }

    if (!nodes.at(toSID).portmapping.contains(to_port)) {
      throw std::runtime_error(get_add_line_err2(
          std::move(fromSID), std::move(toSID), std::move(to_port)));
    }

    nodes.at(toSID).ins[nodes.at(toSID).portmapping.at(to_port)].fromSID =
        std::move(fromSID);
  }

  auto traverse(const Node &node, vector<target::Operation> &steps,
                vector<target::Operation> &delays,
                unordered_set<string> &visited,
                unordered_set<string> &in_progress) -> void {
    using enum Node::Type;
    using namespace target;

    if (in_progress.contains(node.SID)) {
      throw runtime_error(
          format("Problem during traverse a graph: we recieved node SID '{}', "
                 "but this SID is already in progress. Unexpected situation, "
                 "interrupting.",
                 node.SID));
    }

    if (visited.contains(node.SID)) {
      return; // we have already visited this node, but this can happen i.e. in
              // diamond case
    }
    if (UnitDelay == node.type) {
      delays.emplace_back(Fab::op_delay(node.name, node.ins[0].sign,
                                        node_name(node.ins[0].fromSID)));
    } else {
      in_progress.insert(node.SID);

      for (const auto &in : node.ins) {
        traverse(nodes.at(in.fromSID), steps, delays, visited, in_progress);
      }

      if (Sum == node.type) {
        steps.emplace_back(Fab::op_sum(
            node.name, node.ins[0].sign, node_name(node.ins[0].fromSID),
            node.ins[1].sign, node_name(node.ins[1].fromSID)));
      } else if (Gain == node.type) {
        steps.emplace_back(Fab::op_gain(node.name, node.ins[0].sign,
                                        node_name(node.ins[0].fromSID),
                                        node.gain));
      } else if (Inport == node.type) {
        // just skip, no node need to calculate when we face Inport
      } else if (Outport == node.type) {

      } else {
        // impossible case: node type is UnitDelay or
        // a new node type has been added
        throw runtime_error(
            format("Unexpected node type faced '{}'. Interrupting.",
                   Node::to_string(node.type)));
      }
    }

    for (auto &in : node.ins) {
      in.fromSID
    }
    * /
  }

  auto transform(const target::IWriter &writer) const -> void {
    using namespace cgen::target;

    vector<InitField> init_fields = {Fab::init_field("UnitDelay1", 0)};
    vector<Operation> step_fields = {
        Fab::op("Add1", "setpoint", "-", "feedback"),
        Fab::op("I_gain", "Add1", "*", 2),
        Fab::op("Ts", "I_gain", "*", 0.01),
        Fab::op("P_gain", "Add1", "*", 3),
        Fab::op("Add2", "Ts", "+", "UnitDelay1"),
        Fab::op("Add3", "P_gain", "+", "Add2"),
        Fab::op("UnitDelay1", "Add2")};

    File expr = Fab::test_file({{"command", {"&nwocg.Add3", 0}},
                                {"feedback", {"&nwocg.feedback", 1}},
                                {"setpoint", {"&nwocg.setpoint", 1}}},
                               {"setpoint", "feedback", "Add1", "I_gain", "Ts",
                                "P_gain", "UnitDelay1", "Add2", "Add3"},
                               init_fields, step_fields);

    writer.write(expr);
  }

private:
  unordered_map<string, Node> nodes;
  string outportSID;
  unordered_set<string> inportSIDs;

  inline auto get_add_line_err1(string fromSID, string toSID, string to_port,
                                string SID) const -> string {
    return format(
        "We try to add line from node SID '{}' to node SID '{}' (port '{}'), "
        "but we have not faced SID '{}' yet. Interrupting.",
        std::move(fromSID), std::move(toSID), std::move(to_port),
        std::move(SID));
  }

  inline auto get_add_line_err2(string fromSID, string toSID,
                                string to_port) const -> string {
    return format("We try to add line from node SID '{}' to node SID '{}' "
                  "(port '{}'), but node with SID '{}' does not contain port "
                  "with name '{}'. Interrupting.",
                  std::move(fromSID), std::move(toSID), std::move(to_port),
                  std::move(toSID), std::move(to_port));
  }

  auto add_node(Node &&node) -> void {
    if (nodes.contains(node.SID)) {
      throw std::runtime_error(
          format("Problem during building a graph: node with SID '{}' already "
                 "exists! Node info: {}",
                 node.SID, node.node_err_info()));
    }
    if (node.portmapping.empty()) {
      node.add_inport("1", 1); // we do always have a port with name '1'
    }
    nodes.insert({node.SID, std::move(node)});
  };

  auto node_name(string SID) -> string { return nodes.at(SID).name; }
};
} // namespace cgen::source
