#pragma once
#include <prettyprint.h>
#include <string>
#include <variant>
#include <vector>

using namespace std;

namespace cgen::target {

class AccessKey {
  friend class Fab;
  AccessKey() = default;
};

struct RawLiteral {
  explicit RawLiteral(AccessKey, string v) : value(std::move(v)) {}
  string value;
};

struct StringLiteral {
  explicit StringLiteral(AccessKey, string v) : value(std::move(v)) {}
  string value;
};

struct PPDoc {
  explicit PPDoc(AccessKey, shared_ptr<pp::doc> doc) : doc(std::move(doc)) {}
  shared_ptr<pp::doc> doc;
};

struct StructInit;
struct ArrayInit;
struct File;
struct PStruct;

using Expr = variant<RawLiteral, StringLiteral, PPDoc, File, StructInit,
                     ArrayInit, PStruct>;

struct File {
  explicit File(AccessKey, vector<Expr> exprs) : exprs(std::move(exprs)) {}
  vector<Expr> exprs;
};

struct StructInit {
  explicit StructInit(AccessKey, vector<Expr> fields)
      : fields(std::move(fields)) {}
  vector<Expr> fields;
};

struct ArrayInit {
  explicit ArrayInit(AccessKey, string type, string name, bool s, bool c,
                     vector<Expr> elements)
      : type_name(std::move(type)), variable_name(std::move(name)),
        is_static(s), is_const(c), elements(std::move(elements)) {}
  string type_name;
  string variable_name;
  bool is_static = true;
  bool is_const = true;
  vector<Expr> elements;
};

struct PStruct {
  explicit PStruct(AccessKey, string name, bool s, vector<Expr> fields)
      : variable_name(std::move(name)), is_static(s),
        fields(std::move(fields)) {}
  string variable_name;
  bool is_static = true;
  vector<Expr> fields;
};

class Fab {
public:
  static inline auto str(string v) -> StringLiteral {
    return StringLiteral{AccessKey{}, std::move(v)};
  }

  static inline auto inum(int v) -> RawLiteral {
    return RawLiteral{AccessKey{}, to_string(v)};
  }

  static inline auto ppdoc(const shared_ptr<pp::doc> &doc) -> PPDoc {
    return PPDoc{AccessKey{}, doc};
  }

  static inline auto ext_ports(const vector<Expr> &elements) -> ArrayInit {
    return ArrayInit{AccessKey{}, "nwocg_ExtPort", "ext_ports", true,
                     true,        elements};
  }

  static auto pstruct(const vector<string> &fields) -> PStruct {
    vector<Expr> flds;
    for (const auto &f : fields) {
      flds.emplace_back(raw("double " + f + ";"));
    }
    return PStruct{AccessKey{}, "nwocg", true, flds};
  }

  static auto
  test_file(const vector<pair<string, pair<string, int>>> &ext_ports_elems,
            const vector<string> &pstruct_fields) -> File {
    vector<Expr> elems;
    for (const auto &e : ext_ports_elems) {
      elems.emplace_back(
          row({str(e.first), raw(e.second.first), inum(e.second.second)}));
    }
    elems.emplace_back(row({inum(0), inum(0), inum(0)}));
    vector<Expr> exprs = {raw("#include \"nwocg_run.h\""),
                          raw("#include <math.h>")};

    exprs.emplace_back(pstruct(pstruct_fields));

    exprs.emplace_back(ext_ports(elems));

    auto gen_ext_ports =
        pp::group(pp::text("const nwocg_ExtPort * const") + pp::line_or(" ")) +
        pp::group(pp::text("nwocg_generated_ext_ports = ext_ports;"));
    exprs.emplace_back(ppdoc(gen_ext_ports));
    auto gen_ext_ports_size =
        pp::group(pp::text("const size_t") + pp::line_or(" ")) +
        pp::group(pp::text("nwocg_generated_ext_ports_size =") +
                  pp::line_or(" ")) +
        pp::group(pp::text("sizeof(ext_ports)"));
    exprs.emplace_back(ppdoc(gen_ext_ports_size));
    return File{AccessKey{}, exprs};
  }

private:
  static inline auto row(vector<Expr> fields) -> StructInit {
    return StructInit{AccessKey{}, std::move(fields)};
  }

  static inline auto raw(string v) -> RawLiteral {
    return RawLiteral{AccessKey{}, std::move(v)};
  }
};

} // namespace cgen::target
