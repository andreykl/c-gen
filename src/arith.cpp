#include <c-gen/target/core/arith.hpp>
#include <format>

using namespace std;

namespace cgen::target::core::arith {

struct is_neg_visitor {
  auto operator()(const Neg &ae) const -> pair<bool, AExpr> {
    return {true, ae.aexpr};
  }
  template <typename AE>
  auto operator()(const AE &ae) const -> pair<bool, AExpr> {
    return {false, ae};
  }
};

struct to_string_visitor {
  auto operator()(const StringLit &aexpr) const -> string {
    return aexpr.value;
  }
  auto operator()(const OneLit &) const -> string { return "1"; }
  auto operator()(const MinusOneLit &) const -> string { return "-1"; }
  auto operator()(const DoubleLit &aexpr) const -> string {
    return format("{}", aexpr.value);
  }
  auto operator()(const Neg &aexpr) const -> string {
    return format("-{}", boost::apply_visitor(*this, aexpr.aexpr));
  }
  auto operator()(const Sum &aexpr) const -> string {
    auto lhs = boost::apply_visitor(*this, aexpr.left);
    auto [is_neg, rhse] = boost::apply_visitor(is_neg_visitor{}, aexpr.right);
    auto rhs = boost::apply_visitor(*this, rhse);
    if (is_neg) {
      return format("{} - {}", lhs, rhs);
    }
    return format("{} + {}", lhs, rhs);
  }
  auto operator()(const Mul &aexpr) const -> string {
    return format("{} * {}", boost::apply_visitor(*this, aexpr.left),
                  boost::apply_visitor(*this, aexpr.right));
  }
};

struct is_one_visitor {
  auto operator()(const OneLit &) const -> bool { return true; }
  template <typename AE> auto operator()(const AE &) const -> bool {
    return false;
  }
};

struct is_minus_one_visitor {
  auto operator()(const MinusOneLit &) const -> bool { return true; }
  template <typename AE> auto operator()(const AE &) const -> bool {
    return false;
  }
};

struct simplify_visitor {
  auto operator()(const Sum &aexpr) const -> AExpr {
    auto lhs = boost::apply_visitor(*this, aexpr.left);
    auto rhs = boost::apply_visitor(*this, aexpr.right);
    return Fab::sum(lhs, rhs);
  }
  auto operator()(const Mul &aexpr) const -> AExpr {
    auto lhs = boost::apply_visitor(*this, aexpr.left);
    auto rhs = boost::apply_visitor(*this, aexpr.right);
    if (boost::apply_visitor(is_one_visitor{}, lhs)) {
      return rhs;
    }
    if (boost::apply_visitor(is_minus_one_visitor{}, lhs)) {
      return Fab::neg(rhs);
    }
    return Fab::mul(lhs, rhs);
  }
  auto operator()(const Neg &aexpr) const -> AExpr {
    auto ae = boost::apply_visitor(*this, aexpr.aexpr);
    return Fab::neg(ae);
  }
  template <typename E> auto operator()(const E &ae) const -> AExpr {
    return ae;
  }
};

auto Fab::simplify(const AExpr &aexpr) -> AExpr {
  return boost::apply_visitor(simplify_visitor{}, aexpr);
}

auto Fab::to_string(const AExpr &aexpr) -> string {
  return boost::apply_visitor(to_string_visitor{}, aexpr);
}
} // namespace cgen::target::core::arith
