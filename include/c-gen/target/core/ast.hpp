#pragma once
#include <c-gen/target/common/concepts.hpp>
#include <vector>

#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>

using namespace std;

namespace cgen::target {

using namespace common::concepts;

namespace api {
class Fab;
}

namespace core {

class AccessKey {
  friend class api::Fab;
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
  explicit File(AccessKey, RangeOf<Statement> auto &&stmts)
      : stmts(ranges::begin(stmts), ranges::end(stmts)) {}
  vector<Statement> stmts;
};

struct StructDecl {
  struct Field {
    Expr type;
    string name;
  };
  explicit StructDecl(AccessKey, RangeOf<Field> auto &&fields)
      : fields(ranges::begin(fields), ranges::end(fields)) {}
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
  explicit ListInit(AccessKey, RangeOf<Expr> auto &&elements)
      : elements(ranges::begin(elements), ranges::end(elements)) {}
  vector<Expr> elements;
};

struct FunDecl {
  explicit FunDecl(AccessKey, Expr type, string name, RangeOf<Expr> auto &&body)
      : type(std::move(type)), name(std::move(name)),
        body(ranges::begin(body), ranges::end(body)) {}
  Expr type;
  string name;
  vector<Expr> body;
};
} // namespace core
} // namespace cgen::target
