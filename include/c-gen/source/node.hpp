#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace cgen::source {
using namespace std;

auto node_err_info(string type, string name, string SID) -> string;

class Node {
  friend class Graph;

public:
  auto inline err_info() const -> string {
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

  static auto inline to_string(Type t) -> string {
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

  auto add_inport(string name, int sign) -> void;
};
} // namespace cgen::source
