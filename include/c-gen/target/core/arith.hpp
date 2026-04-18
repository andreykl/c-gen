#pragma once
#include <boost/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <string>

using namespace std;

namespace cgen::target::core::arith {

class AccessKey {
  friend class Fab;
  AccessKey() = default;
};

struct StringLit {
  explicit StringLit(AccessKey, string value) : value(std::move(value)){};
  string value;
};

struct OneLit {
  explicit OneLit(AccessKey) {}
};

struct MinusOneLit {
  explicit MinusOneLit(AccessKey) {}
};

struct DoubleLit {
  explicit DoubleLit(AccessKey, double value) : value(value) {}
  double value;
};

struct Neg;
struct Sum;
struct Mul;

using AExpr =
    boost::variant<StringLit, OneLit, MinusOneLit, DoubleLit,
                   boost::recursive_wrapper<Neg>, boost::recursive_wrapper<Sum>,
                   boost::recursive_wrapper<Mul>>;

struct Neg {
  explicit Neg(AccessKey, AExpr aexpr) : aexpr(std::move(aexpr)){};
  AExpr aexpr;
};

struct Sum {
  explicit Sum(AccessKey, AExpr left, AExpr right)
      : left(std::move(left)), right(std::move(right)) {}
  AExpr left, right;
};

struct Mul {
  explicit Mul(AccessKey, AExpr left, AExpr right)
      : left(std::move(left)), right(std::move(right)){};
  AExpr left, right;
};

class Fab {
public:
  static inline auto str(string e) -> AExpr {
    return StringLit{AccessKey{}, std::move(e)};
  }
  static inline auto one() -> AExpr { return OneLit{AccessKey{}}; }
  static inline auto mone() -> AExpr { return MinusOneLit{AccessKey{}}; }
  static inline auto dble(double val) -> AExpr {
    return DoubleLit{AccessKey{}, val};
  }
  static inline auto neg(AExpr e) -> AExpr {
    return Neg{AccessKey{}, std::move(e)};
  }
  static inline auto sum(AExpr lhs, AExpr rhs) -> AExpr {
    return Sum{AccessKey{}, std::move(lhs), std::move(rhs)};
  }
  static inline auto mul(AExpr lhs, AExpr rhs) -> AExpr {
    return Mul{AccessKey{}, std::move(lhs), std::move(rhs)};
  }

  static auto simplify(const AExpr &) -> AExpr;
  static auto to_string(const AExpr &) -> string;
};
} // namespace cgen::target::core::arith
