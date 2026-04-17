#pragma once
#include <c-gen/target/api/iwriter.hpp>
#include <c-gen/target/api/structs.hpp>
#include <c-gen/target/core/ast.hpp>
#include <format>
#include <string>

namespace cgen::target::api {

using namespace std;

class Fab {
public:
  static inline auto inport(string port_name, string field, int sign) -> Port {
    return Port{AccessKey{},
                row({str(std::move(port_name)),
                     raw(format("&{}", nwf(std::move(field)))), inum(sign)})};
  }

  static inline auto outport(string port_name, string field) -> Port {
    return Port{AccessKey{},
                row({str(std::move(port_name)),
                     raw(format("&{}", nwf(std::move(field)))), inum(0)})};
  }

  static inline auto op_sum(string lfield, int rlsign, string rlfield,
                            int rrsign, string rrfield) -> Operation {
    auto rhs = raw(format("{} * {} + {} * {}", rlsign, nwf(std::move(rlfield)),
                          rrsign, nwf(std::move(rrfield))));
    return Operation{AccessKey{},
                     Assignment{AccessKey{}, raw(nwf(std::move(lfield))), rhs}};
  }

  static inline auto op_gain(string lfield, int rlsign, string rlfield,
                             double val) -> Operation {
    auto rhs =
        raw(format("{} * {} * {}", rlsign, nwf(std::move(rlfield)), val));
    return Operation{AccessKey{},
                     Assignment{AccessKey{}, raw(nwf(std::move(lfield))), rhs}};
  }

  static inline auto op_delay(string lfield, int sign,
                              string rfield) -> Operation {
    return Operation{
        AccessKey{},
        Assignment{AccessKey{}, raw(nwf(std::move(lfield))),
                   raw(format("{} * {}", sign, nwf(std::move(rfield))))}};
  }

  static inline auto init_field(string field, const double val) -> InitField {
    return InitField{AccessKey{},
                     Assignment{AccessKey{}, raw(nwf(std::move(field))),
                                raw(format("{}", val))}};
  }

  static inline auto file(const vector<Port> &ext_ports_elems,
                          const vector<string> &pstruct_fields,
                          const vector<InitField> &init_fields,
                          const vector<Operation> &step_fields) -> File {
    vector<Expr> elems;
    for (const auto &e : ext_ports_elems) {
      elems.emplace_back(e.value);
    }
    elems.emplace_back(row({inum(0), inum(0), inum(0)}));

    vector<core::File::Statement> stmts = {include("#include \"nwocg_run.h\""),
                                           include("#include <math.h>", true)};

    stmts.emplace_back(true, true, pstruct(pstruct_fields));
    stmts.emplace_back(false, true, generated_init(init_fields));
    stmts.emplace_back(false, true, generated_step(step_fields));

    stmts.emplace_back(true, true, ext_ports(std::move(elems)));

    auto gen_ext_ports =
        assign(var_decl(raw("nwocg_ExtPort * const"),
                        "nwocg_generated_ext_ports", false, true),
               raw("ext_ports"));
    stmts.emplace_back(true, true, std::move(gen_ext_ports));

    auto gen_ext_ports_size = assign(
        var_decl(raw("size_t"), "nwocg_generated_ext_ports_size", false, true),
        raw("sizeof(ext_ports)"));
    stmts.emplace_back(true, false, std::move(gen_ext_ports_size));

    return File{AccessKey{}, core::File{AccessKey{}, std::move(stmts)}};
  }

private:
  static inline auto
  generated_init(const vector<InitField> &init_fields) -> FunDecl {
    vector<Expr> body;
    for (const auto &f : init_fields)
      body.emplace_back(f.value);

    return fun_decl(raw("void"), "nwocg_generated_init", std::move(body));
  }

  static inline auto
  generated_step(const vector<Operation> &step_fields) -> FunDecl {
    vector<Expr> body;
    for (const auto &f : step_fields)
      body.emplace_back(f.value);

    return fun_decl(raw("void"), "nwocg_generated_step", std::move(body));
  }

  static inline auto nwf(string f) -> string { return "nwocg." + f; }

  static inline auto row(vector<Expr> fields) -> ListInit {
    return ListInit{AccessKey{}, std::move(fields)};
  }

  static inline auto raw(string v) -> RawLiteral {
    return RawLiteral{AccessKey{}, std::move(v)};
  }

  static inline auto include(string s,
                             bool linebreak = false) -> core::File::Statement {
    return core::File::Statement{false, linebreak, raw(std::move(s))};
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

  static inline auto ext_ports(vector<Expr> elements) -> Assignment {
    return assign(var_decl(raw("nwocg_ExtPort"), "ext_ports[]", true, true),
                  list_init(std::move(elements)));
  }

  static inline auto pstruct(const vector<string> &fields) -> VarDecl {
    vector<StructDecl::Field> flds;
    for (const auto &f : fields) {
      flds.push_back({raw("double"), f});
    }
    return var_decl(struct_decl(std::move(flds)), "nwocg", true, false);
  }

  static inline auto fun_decl(Expr type, string name,
                              vector<Expr> body) -> FunDecl {
    return FunDecl{AccessKey{}, std::move(type), std::move(name),
                   std::move(body)};
  }
};
} // namespace cgen::target::api
