#include <c-gen/target/ast.hpp>
#include <ranges>
#include <unordered_map>
#include <vector>

using namespace std;

namespace cgen::source {
auto node_err_info(const string &type, const string &name,
                   const string &SID) -> string {
  return "name = '" + name + "', type = '" + type + "', SID = '" + SID + "'";
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
    bool color = false;
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

    auto add_inport(const string &name, const int sign) -> void {
      if (portmapping.contains(name)) {
        throw std::runtime_error("Problem during building a graph: portmapping "
                                 "already contains port with name '" +
                                 name + "'. Interrupting.");
      }
      portmapping[name] = ins.size();
      ins.emplace_back("", sign);
    }
  };

public:
  inline auto add_inport(const string &name, const string &SID) -> void {
    add_node(SID, Node{Node::Type::Inport, name, SID});
  };

  auto add_sum(const string &name, const string &SID,
               const vector<string> &portnames,
               const vector<int> &portsigns) -> void {

    vector<int> fallback;
    if (portsigns.empty())
      fallback.assign(portnames.size(), 1);

    auto &signs = portsigns.empty() ? fallback : portsigns;

    if (signs.size() != portnames.size())
      throw std::runtime_error(
          "Problem during building a graph: portsigns are not empty and "
          "portsigns.size != portname.size. Node info: " +
          node_err_info(Node::to_string(Node::Type::Sum), name, SID));

    auto node = Node{Node::Type::Sum, name, SID};

    for (auto &&[name, sign] : views::zip(portnames, signs)) {
      node.add_inport(name, sign);
    }

    add_node(SID, std::move(node));
  };

  inline auto add_gain(const string &name, const string &SID,
                       const double gain) -> void {
    add_node(SID, Node{Node::Type::Gain, name, SID, gain});
  }

  inline auto add_unit_delay(const string &name, const string &SID,
                             const int sample_time) -> void {
    add_node(SID, Node{Node::Type::UnitDelay, name, SID, sample_time});
  }

  inline auto add_outport(const string &name, const string &SID) -> void {
    add_node(SID, Node{Node::Type::Outport, name, SID});
    outportSID = SID;
  }

  auto add_line(const string &fromSID, const string &toSID,
                const string to_port) -> void {
    if (!nodes.contains(toSID)) {
      throw std::runtime_error(
          get_add_line_err1(fromSID, toSID, to_port, toSID));
    }

    if (!nodes.contains(fromSID)) {
      throw std::runtime_error(
          get_add_line_err1(fromSID, toSID, to_port, fromSID));
    }

    if (!nodes.at(toSID).portmapping.contains(to_port)) {
      throw std::runtime_error(get_add_line_err2(fromSID, toSID, to_port));
    }

    nodes.at(toSID).ins[nodes.at(toSID).portmapping.at(to_port)].fromSID =
        fromSID;
  }

  auto transform() -> cgen::target::File {
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

    return expr;
  }

private:
  unordered_map<string, Node> nodes;
  string outportSID;

  inline auto get_add_line_err1(const string &fromSID, const string &toSID,
                                const string &to_port,
                                const string &SID) -> string {
    return "We try to add line from node SID '" + fromSID + "' to node SID " +
           toSID + " (port '" + to_port + "'), but we have not faced SID '" +
           SID + "' yet. Interrupting.";
  }

  inline auto get_add_line_err2(const string &fromSID, const string &toSID,
                                const string &to_port) -> string {
    return "We try to add line from node SID '" + fromSID + "' to node SID " +
           toSID + " (port '" + to_port + "'), but node with SID '" + toSID +
           "' does not contain port with name '" + to_port + "'. Interrupting.";
  }

  auto add_node(const string &SID, Node &&node) -> void {
    if (nodes.contains(SID)) {
      throw std::runtime_error(
          "Problem during building a graph: node with SID '" + SID +
          "' already exists! Node info: " + node.node_err_info());
    }
    if (node.portmapping.empty()) {
      node.add_inport("1", 1); // we do always have a port with name '1'
    }
    nodes.insert({SID, node});
  };
};
} // namespace cgen::source
