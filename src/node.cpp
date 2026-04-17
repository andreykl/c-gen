#include <c-gen/source/node.hpp>
#include <format>

namespace cgen::source {
auto node_err_info(string type, string name, string SID) -> string {
  return format("name = '{}', type = '{}', SID = '{}'", std::move(name),
                std::move(type), std::move(SID));
}

auto Node::add_inport(string name, int sign) -> void {
  if (portmapping.contains(name)) {
    throw std::runtime_error(
        format("Problem during building a graph: portmapping "
               "already contains port with name '{}'. Interrupting.",
               std::move(name)));
  }
  portmapping.insert({std::move(name), ins.size()});
  ins.emplace_back("", sign);
}

} // namespace cgen::source
