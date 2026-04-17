#include <c-gen/source/graph.hpp>
#include <format>
#include <ranges>

namespace cgen::source {
auto Graph::add_inport(string name, string SID) -> void {
  add_node(Node{Node::Type::Inport, std::move(name), SID});
  inportSIDs.insert(std::move(SID));
};

auto Graph::add_sum(string name, string SID, const vector<string> &portnames,
                    const vector<int> &portsigns) -> void {

  vector<int> fallback;
  if (portsigns.empty())
    fallback.assign(portnames.size(), 1);

  auto &signs = portsigns.empty() ? fallback : portsigns;

  if (signs.size() != portnames.size())
    throw runtime_error(
        format("Problem during building a graph: portsigns are not empty and "
               "portsigns.size != portname.size. Node info: {}. Interrupting.",
               node_err_info(Node::to_string(Node::Type::Sum), std::move(name),
                             std::move(SID))));

  if (portnames.size() != 2) {
    throw runtime_error(
        format("Problem during building a graph: we must have 2 ports for "
               "Sum type node, but we have {}. Node info: {}. Interrupting.",
               portnames.size(),
               node_err_info(Node::to_string(Node::Type::Sum), std::move(name),
                             std::move(SID))));
  }

  auto node = Node{Node::Type::Sum, std::move(name), std::move(SID)};

  for (auto &&[name, sign] : views::zip(portnames, signs)) {
    node.add_inport(name, sign);
  }

  add_node(std::move(node));
};

auto Graph::add_gain(string name, string SID, double gain) -> void {
  add_node(Node{Node::Type::Gain, std::move(name), std::move(SID), gain});
}

auto Graph::add_unit_delay(string name, string SID, int sample_time) -> void {
  add_node(Node{Node::Type::UnitDelay, std::move(name), std::move(SID),
                sample_time});
}

auto Graph::add_outport(string name, string SID) -> void {
  add_node(Node{Node::Type::Outport, std::move(name), SID});
  outportSID = std::move(SID);
}

auto Graph::add_line(string fromSID, string toSID,
                     size_t to_port_ind /* index of port in ins */) -> void {
  if (!nodes.contains(toSID)) {
    throw std::runtime_error(get_add_line_err1(
        std::move(fromSID), std::move(toSID), to_port_ind, std::move(toSID)));
  }

  if (!nodes.contains(fromSID)) {
    throw std::runtime_error(get_add_line_err1(
        std::move(fromSID), std::move(toSID), to_port_ind, std::move(fromSID)));
  }

  if (nodes.at(toSID).ins.size() - 1 < to_port_ind) {
    throw std::runtime_error(
        get_add_line_err2(std::move(fromSID), std::move(toSID), to_port_ind));
  }

  nodes.at(toSID).ins[to_port_ind].fromSID = std::move(fromSID);
}

auto Graph::transform(const IWriter &writer) const -> void {
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

auto Graph::traverse(const Node &node, vector<Operation> &steps,
                     vector<Operation> &delays, vector<Port> &ports,
                     vector<string> &struct_fields,
                     vector<InitField> &init_fields,
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
      // impossible case: a new node type has been added
      throw runtime_error(
          format("Unexpected node type faced '{}'. Interrupting.",
                 Node::to_string(node.type)));
    }

    in_progress.erase(node.SID);
    visited.insert(node.SID);
  }
}

auto Graph::get_add_line_err1(string fromSID, string toSID, int to_port_ind,
                              string SID) const -> string {
  return format(
      "We try to add line from node SID '{}' to node SID '{}' (port '{}'), "
      "but we have not faced SID '{}' yet. Interrupting.",
      std::move(fromSID), std::move(toSID), to_port_ind, std::move(SID));
}

auto Graph::get_add_line_err2(string fromSID, string toSID,
                              int to_port_ind) const -> string {
  return format("We try to add line from node SID '{}' to node SID '{}' "
                "(port '{}'), but node with SID '{}' does not contain port "
                "with name '{}'. Interrupting.",
                std::move(fromSID), std::move(toSID), to_port_ind,
                std::move(toSID), to_port_ind);
}

auto Graph::add_node(Node &&node) -> void {
  if (nodes.contains(node.SID)) {
    throw std::runtime_error(
        format("Problem during building a graph: node with SID '{}' already "
               "exists! Node info: {}",
               node.SID, node.err_info()));
  }
  if (node.portmapping.empty()) {
    node.add_inport("1", 1); // we do always have a port with name '1'
  }
  nodes.insert({node.SID, std::move(node)});
};

auto Graph::node_name_cl(string SID) const -> string {
  return clean(nodes.at(SID).name);
}

auto Graph::clean(string name) const -> string {
  std::erase_if(name, [](unsigned char c) { return std::isspace(c); });
  return name;
}
} // namespace cgen::source
