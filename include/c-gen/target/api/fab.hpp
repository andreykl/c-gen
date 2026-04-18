#pragma once
#include <c-gen/target/api/iwriter.hpp>
#include <c-gen/target/api/structs.hpp>
#include <c-gen/target/common/concepts.hpp>
#include <c-gen/target/core/arith.hpp>
#include <c-gen/target/core/ast.hpp>
#include <format>
#include <ranges>
#include <string>

namespace cgen::target::api {

using namespace std;
using namespace common::concepts;

class Fab {
public:
  static inline auto inport(string port_name, string field, int sign) -> Port {
    return Port{AccessKey{},
                row(vector<Expr>{str(std::move(port_name)),
                                 raw(format("&{}", nwf(std::move(field)))),
                                 inum(sign)})};
  }

  static inline auto outport(string port_name, string field) -> Port {
    return Port{
        AccessKey{},
        row(vector<Expr>{str(std::move(port_name)),
                         raw(format("&{}", nwf(std::move(field)))), inum(0)})};
  }

  static inline auto op_sum(string lfield, int rlsign, string rlfield,
                            int rrsign, string rrfield) -> Operation {
    auto rhs = arith::Fab::sum(
        arith::Fab::mul(asign(rlsign),
                        arith::Fab::str(nwf(std::move(rlfield)))),
        arith::Fab::mul(asign(rrsign),
                        arith::Fab::str(nwf(std::move(rrfield)))));
    return Operation{AccessKey{}, raw(nwf(std::move(lfield))), rhs};
  }

  static inline auto op_gain(string lfield, int rlsign, string rlfield,
                             double val) -> Operation {
    auto rhs = arith::Fab::mul(
        asign(rlsign), arith::Fab::mul(arith::Fab::str(nwf(std::move(rlfield))),
                                       arith::Fab::dble(val)));
    return Operation{AccessKey{}, raw(nwf(std::move(lfield))), rhs};
  }

  static inline auto op_delay(string lfield, int sign,
                              string rfield) -> Operation {
    return Operation{
        AccessKey{}, raw(nwf(std::move(lfield))),
        arith::Fab::mul(asign(sign), arith::Fab::str(nwf(std::move(rfield))))};
  };

  static inline auto init_field(string field, const double val) -> InitField {
    return InitField{AccessKey{},
                     Assignment{AccessKey{}, raw(nwf(std::move(field))),
                                raw(format("{}", val))}};
  }

  static inline auto file(RangeOf<Port> auto &&ext_ports_elems,
                          RangeOf<string> auto &&pstruct_fields,
                          RangeOf<InitField> auto &&init_fields,
                          RangeOf<Operation> auto &&step_fields) -> File {
    auto elems = ext_ports_elems | std::views::transform(&Port::value) |
                 ranges::to<vector>();

    elems.emplace_back(row(vector<Expr>{inum(0), inum(0), inum(0)}));

    vector<core::File::Statement> stmts = {include("#include \"nwocg_run.h\""),
                                           include("#include <math.h>", true)};

    stmts.emplace_back(true, true, pstruct(pstruct_fields));
    stmts.emplace_back(false, true, generated_init(init_fields));
    stmts.emplace_back(false, true, generated_step(step_fields));

    stmts.emplace_back(true, true, ext_ports(elems));

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
  static inline auto asign(int sign) -> core::arith::AExpr {
    return (1 == sign) ? core::arith::AExpr{arith::Fab::one()}
                       : core::arith::AExpr{arith::Fab::mone()};
  }

  static inline auto
  generated_init(RangeOf<InitField> auto &&init_fields) -> FunDecl {
    return fun_decl(raw("void"), "nwocg_generated_init",
                    init_fields | views::transform(&InitField::value));
  }

  static inline auto
  generated_step(RangeOf<Operation> auto &&step_fields) -> FunDecl {
    return fun_decl(raw("void"), "nwocg_generated_step",
                    step_fields | views::transform([](const Operation &op) {
                      auto rhs =
                          arith::Fab::to_string(arith::Fab::simplify(op.rhs));
                      return Assignment{AccessKey{}, op.lhs, raw(rhs)};
                    }));
  }

  static inline auto nwf(string f) -> string { return "nwocg." + f; }

  static inline auto row(RangeOf<Expr> auto &&fields) -> ListInit {
    return ListInit{AccessKey{}, fields};
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

  static inline auto list_init(RangeOf<Expr> auto &&elements) -> ListInit {
    return ListInit{AccessKey{}, std::move(elements)};
  }

  static inline auto
  struct_decl(RangeOf<StructDecl::Field> auto &&fields) -> StructDecl {
    return StructDecl{AccessKey{}, fields};
  }

  static inline auto ext_ports(RangeOf<Expr> auto &&elements) -> Assignment {
    return assign(var_decl(raw("nwocg_ExtPort"), "ext_ports[]", true, true),
                  list_init(elements));
  }

  static inline auto pstruct(RangeOf<string> auto &&fields) -> VarDecl {
    return var_decl(struct_decl(fields | views::transform([](const auto &f) {
                                  return StructDecl::Field{raw("double"), f};
                                })),
                    "nwocg", true, false);
  }

  static inline auto fun_decl(Expr type, string name,
                              RangeOf<Expr> auto &&body) -> FunDecl {
    return FunDecl{AccessKey{}, std::move(type), std::move(name), body};
  }
};
} // namespace cgen::target::api
