#include <c-gen/target/ast.hpp>
#include <c-gen/target/iwriter.hpp>
#include <format>
// #include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace cgen::target::api;

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

  auto add_line(string fromSID, string toSID,
                int to_port_ind /* index of port in ins */) -> void {
    if (!nodes.contains(toSID)) {
      throw std::runtime_error(get_add_line_err1(
          std::move(fromSID), std::move(toSID), to_port_ind, std::move(toSID)));
    }

    if (!nodes.contains(fromSID)) {
      throw std::runtime_error(get_add_line_err1(std::move(fromSID),
                                                 std::move(toSID), to_port_ind,
                                                 std::move(fromSID)));
    }

    if (nodes.at(toSID).ins.size() - 1 < to_port_ind) {
      throw std::runtime_error(
          get_add_line_err2(std::move(fromSID), std::move(toSID), to_port_ind));
    }

    nodes.at(toSID).ins[to_port_ind].fromSID = std::move(fromSID);
  }

  auto traverse(const Node &node, vector<Operation> &steps,
                vector<Operation> &delays, vector<Port> &ports,
                vector<string> &struct_fields, vector<InitField> &init_fields,
                unordered_set<string> &visited,
                unordered_set<string> &in_progress) const -> void {
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

    // std::cout << "here\n";

    if (UnitDelay == node.type) {
      auto nname = clean(node.name);
      delays.emplace_back(Fab::op_delay(nname, node.ins[0].sign,
                                        node_name_cl(node.ins[0].fromSID)));
      struct_fields.push_back(nname);
      init_fields.push_back(Fab::init_field(nname, 0));
    } else {
      in_progress.insert(node.SID);

      if (Inport != node.type) {
        for (const auto &in : node.ins) {
          traverse(nodes.at(in.fromSID), steps, delays, ports, struct_fields,
                   init_fields, visited, in_progress);
        }
      }

      auto nname = clean(node.name);

      if (Sum == node.type) {
        steps.emplace_back(Fab::op_sum(
            nname, node.ins[0].sign, node_name_cl(node.ins[0].fromSID),
            node.ins[1].sign, node_name_cl(node.ins[1].fromSID)));
        struct_fields.push_back(nname);
      } else if (Gain == node.type) {
        steps.emplace_back(Fab::op_gain(nname, node.ins[0].sign,
                                        node_name_cl(node.ins[0].fromSID),
                                        node.gain));
        struct_fields.push_back(nname);
      } else if (Inport == node.type) {
        ports.emplace_back(Fab::inport(nname, nname, node.ins[0].sign));
        struct_fields.push_back(nname);
        // just skip, no node need to calculate when we face Inport
      } else if (Outport == node.type) {
        ports.emplace_back(
            Fab::outport(nname, node_name_cl(node.ins[0].fromSID)));
      } else {
        // impossible case: node type is UnitDelay or
        // a new node type has been added
        throw runtime_error(
            format("Unexpected node type faced '{}'. Interrupting.",
                   Node::to_string(node.type)));
      }

      in_progress.erase(node.SID);
      visited.insert(node.SID);
    }
  }

  auto transform(const target::IWriter &writer) const -> void {
    vector<InitField> init_fields;
    vector<Operation> steps, delays;
    vector<Port> ports;
    vector<string> struct_fields;
    unordered_set<string> visited, in_progress;

    traverse(nodes.at(outportSID), steps, delays, ports, struct_fields,
             init_fields, visited, in_progress);

    vector<Operation> step_fields;
    step_fields.reserve(steps.size() + delays.size());
    step_fields.insert(step_fields.end(), steps.begin(), steps.end());
    step_fields.insert(step_fields.end(), delays.begin(), delays.end());

    vector<Port> reversed_ports(ports.rbegin(), ports.rend());

    writer.write(
        Fab::file(reversed_ports, struct_fields, init_fields, step_fields));
  }

private:
  unordered_map<string, Node> nodes;
  string outportSID;
  unordered_set<string> inportSIDs;

  inline auto get_add_line_err1(string fromSID, string toSID, int to_port_ind,
                                string SID) const -> string {
    return format(
        "We try to add line from node SID '{}' to node SID '{}' (port '{}'), "
        "but we have not faced SID '{}' yet. Interrupting.",
        std::move(fromSID), std::move(toSID), to_port_ind, std::move(SID));
  }

  inline auto get_add_line_err2(string fromSID, string toSID,
                                int to_port_ind) const -> string {
    return format("We try to add line from node SID '{}' to node SID '{}' "
                  "(port '{}'), but node with SID '{}' does not contain port "
                  "with name '{}'. Interrupting.",
                  std::move(fromSID), std::move(toSID), to_port_ind,
                  std::move(toSID), to_port_ind);
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

  auto node_name_cl(string SID) const -> string {
    return clean(nodes.at(SID).name);
  }

  auto clean(string name) const -> string {
    std::erase_if(name, [](unsigned char c) { return std::isspace(c); });
    return name;
  }
};
} // namespace cgen::source
