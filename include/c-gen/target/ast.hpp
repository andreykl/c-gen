#pragma once
#include <string>
#include <vector>

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

using namespace std;

namespace cgen::target {

class AccessKey {
  friend class Fab;
  AccessKey() = default;
};

struct RawLiteral {
  explicit RawLiteral(AccessKey, string value) : value(std::move(value)) {}
  string value;
};

struct StringLiteral {
  explicit StringLiteral(AccessKey, string value) : value(std::move(value)) {}
  string value;
};

struct File;
struct StructDecl;
struct Assignment;
struct VarDecl;
struct ListInit;
struct FunDecl;

using Expr = boost::variant<
    RawLiteral, StringLiteral, boost::recursive_wrapper<File>,
    boost::recursive_wrapper<StructDecl>, boost::recursive_wrapper<Assignment>,
    boost::recursive_wrapper<VarDecl>, boost::recursive_wrapper<ListInit>,
    boost::recursive_wrapper<FunDecl>>;

struct File {
  struct Statement {
    explicit Statement(bool semicolon, bool linebreak, Expr expr)
        : semicolon(semicolon), linebreak(linebreak), expr(std::move(expr)) {}
    bool semicolon;
    bool linebreak;
    Expr expr;
  };
  explicit File(AccessKey, vector<Statement> stmts) : stmts(std::move(stmts)) {}
  vector<Statement> stmts;
};

struct StructDecl {
  struct Field {
    Expr type;
    string name;
  };
  explicit StructDecl(AccessKey, vector<Field> fields)
      : fields(std::move(fields)) {}
  vector<Field> fields;
};

struct Assignment {
  explicit Assignment(AccessKey, Expr lhs, Expr rhs)
      : lhs(std::move(lhs)), rhs(std::move(rhs)) {}
  Expr lhs;
  Expr rhs;
};

struct VarDecl {
  explicit VarDecl(AccessKey, Expr type, string name, bool is_static,
                   bool is_const)
      : type(std::move(type)), name(std::move(name)), is_static(is_static),
        is_const(is_const) {}
  Expr type;
  string name;
  bool is_static;
  bool is_const;
};

struct ListInit {
  explicit ListInit(AccessKey, vector<Expr> elements)
      : elements(std::move(elements)) {}
  vector<Expr> elements;
};

struct FunDecl {
  explicit FunDecl(AccessKey, Expr type, string name, vector<Expr> body)
      : type(std::move(type)), name(std::move(name)), body(std::move(body)) {}
  Expr type;
  string name;
  vector<Expr> body;
};

struct Operation {
  explicit Operation(AccessKey, Assignment value) : value(std::move(value)) {}
  Assignment value;
};

struct InitField {
  explicit InitField(AccessKey, Assignment value) : value(std::move(value)) {}
  Assignment value;
};

class Fab {
public:
  static inline auto op(const string &lfield, const string &rlfield,
                        const string &op, const string &rrfield) -> Operation {
    return Operation{AccessKey{}, Assignment{AccessKey{}, raw(nwf(lfield)),
                                             raw(nwf(rlfield) + " " + op + " " +
                                                 nwf(rrfield))}};
  }

  static inline auto op(const string &lfield, const string &rlfield,
                        const string &op, const double val) -> Operation {
    return Operation{AccessKey{}, Assignment{AccessKey{}, raw(nwf(lfield)),
                                             raw(nwf(rlfield) + " " + op + " " +
                                                 to_string(val))}};
  }

  static inline auto op(const string &lfield,
                        const string &rfield) -> Operation {
    return Operation{AccessKey{}, Assignment{AccessKey{}, raw(nwf(lfield)),
                                             raw(nwf(rfield))}};
  }

  static inline auto init_field(const string &field,
                                const double val) -> InitField {
    return InitField{AccessKey{}, Assignment{AccessKey{}, raw(nwf(field)),
                                             raw(to_string(val))}};
  }

  static auto
  test_file(const vector<pair<string, pair<string, int>>> &ext_ports_elems,
            const vector<string> &pstruct_fields,
            const vector<InitField> &init_fields,
            const vector<Operation> &step_fields) -> File {
    vector<Expr> elems;
    for (const auto &e : ext_ports_elems) {
      elems.emplace_back(
          row({str(e.first), raw(e.second.first), inum(e.second.second)}));
    }
    elems.emplace_back(row({inum(0), inum(0), inum(0)}));
    vector<File::Statement> stmts = {include("#include \"nwocg_run.h\""),
                                     include("#include <math.h>", true)};

    stmts.emplace_back(true, true, pstruct(pstruct_fields));
    stmts.emplace_back(false, true, generated_init(init_fields));
    stmts.emplace_back(false, true, generated_step(step_fields));

    stmts.emplace_back(true, true, ext_ports(elems));

    auto gen_ext_ports =
        assign(var_decl(raw("nwocg_ExtPort * const"),
                        "nwocg_generated_ext_ports", false, true),
               raw("ext_ports"));
    stmts.emplace_back(true, true, gen_ext_ports);

    auto gen_ext_ports_size = assign(
        var_decl(raw("size_t"), "nwocg_generated_ext_ports_size", false, true),
        raw("sizeof(ext_ports)"));
    stmts.emplace_back(true, false, gen_ext_ports_size);

    return File{AccessKey{}, stmts};
  }

private:
  static auto generated_init(const vector<InitField> &init_fields) -> FunDecl {
    vector<Expr> body;
    for (const auto &f : init_fields)
      body.emplace_back(f.value);

    return fun_decl(raw("void"), "nwocg_generated_init", body);
  }

  static auto generated_step(const vector<Operation> &step_fields) -> FunDecl {
    vector<Expr> body;
    for (const auto &f : step_fields)
      body.emplace_back(f.value);

    return fun_decl(raw("void"), "nwocg_generated_step", body);
  }

  static inline auto nwf(const string &f) -> string { return "nwocg." + f; }

  static inline auto row(vector<Expr> fields) -> ListInit {
    return ListInit{AccessKey{}, std::move(fields)};
  }

  static inline auto raw(string v) -> RawLiteral {
    return RawLiteral{AccessKey{}, std::move(v)};
  }

  static inline auto include(const string &s,
                             bool linebreak = false) -> File::Statement {
    return File::Statement{false, linebreak, raw(s)};
  }

  static inline auto str(string v) -> StringLiteral {
    return StringLiteral{AccessKey{}, std::move(v)};
  }

  static inline auto inum(int v) -> RawLiteral {
    return RawLiteral{AccessKey{}, to_string(v)};
  }

  static inline auto assign(Expr lhs, Expr rhs) -> Assignment {
    return Assignment{AccessKey{}, std::move(lhs), std::move(rhs)};
  }

  static inline auto var_decl(Expr type, string name, bool is_static = false,
                              bool is_const = false) -> VarDecl {
    return VarDecl{AccessKey{}, std::move(type), std::move(name), is_static,
                   is_const};
  }

  static inline auto list_init(vector<Expr> elements) -> ListInit {
    return ListInit{AccessKey{}, std::move(elements)};
  }

  static inline auto
  struct_decl(vector<StructDecl::Field> fields) -> StructDecl {
    return StructDecl{AccessKey{}, std::move(fields)};
  }

  static inline auto ext_ports(const vector<Expr> &elements) -> Assignment {
    return assign(var_decl(raw("nwocg_ExtPort"), "ext_ports[]", true, true),
                  list_init(elements));
  }

  static auto pstruct(const vector<string> &fields) -> VarDecl {
    vector<StructDecl::Field> flds;
    for (const auto &f : fields) {
      flds.push_back({raw("double"), f});
    }
    return var_decl(struct_decl(flds), "nwocg", true, false);
  }

  static inline auto fun_decl(const Expr &type, const string &name,
                              const vector<Expr> &body) -> FunDecl {
    return FunDecl{AccessKey{}, type, name, body};
  }
};

} // namespace cgen::target
