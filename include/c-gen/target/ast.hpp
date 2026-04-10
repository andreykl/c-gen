#pragma once
#include <string>
#include <variant>
#include <vector>

namespace cgen::target {

class AccessKey {
  friend class Fab;
  AccessKey() = default;
};

struct RawLiteral {
  explicit RawLiteral(AccessKey, std::string v) : value(std::move(v)) {}
  std::string value;
};

struct StringLiteral {
  explicit StringLiteral(AccessKey, std::string v) : value(std::move(v)) {}
  std::string value;
};

struct StructInit;
struct ArrayInit;
struct File;

using Expr =
    std::variant<File, StringLiteral, RawLiteral, StructInit, ArrayInit>;

struct File {
  explicit File(AccessKey, std::vector<Expr> es) : exprs(std::move(es)) {}
  std::vector<Expr> exprs;
};

struct StructInit {
  explicit StructInit(AccessKey, std::vector<Expr> f) : fields(std::move(f)) {}
  std::vector<Expr> fields;
};

struct ArrayInit {
  explicit ArrayInit(AccessKey, std::string type, std::string name, bool s,
                     bool c, std::vector<StructInit> el)
      : type_name(std::move(type)), variable_name(std::move(name)),
        is_static(s), is_const(c), elements(std::move(el)) {}
  std::string type_name;
  std::string variable_name;
  bool is_static = true;
  bool is_const = true;
  std::vector<StructInit> elements;
};

class Fab {
public:
  static inline auto str(std::string v) -> StringLiteral {
    return StringLiteral{AccessKey{}, std::move(v)};
  }

  static inline auto inum(int v) -> RawLiteral {
    return RawLiteral{AccessKey{}, std::to_string(v)};
  }

  static inline auto extPorts() -> ArrayInit {
    auto elements = {row({str("command"), raw("&nwocg.Add3"), inum(0)}),
                     row({str("feedback"), raw("&nwocg.feedback"), inum(1)}),
                     row({str("setpoint"), raw("&nwocg.setpoint"), inum(1)}),
                     row({inum(0), inum(0), inum(0)})};
    return ArrayInit{AccessKey{}, "nwocg_ExtPort", "ext_ports[]", true,
                     true,        elements};
  }

private:
  static inline auto row(std::vector<Expr> fields) -> StructInit {
    return StructInit{AccessKey{}, std::move(fields)};
  }

  static inline auto raw(std::string v) -> RawLiteral {
    return RawLiteral{AccessKey{}, std::move(v)};
  }
};

} // namespace cgen::target
