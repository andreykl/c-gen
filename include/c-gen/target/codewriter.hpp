#pragma once
#include <c-gen/target/ast.hpp>
#include <iostream>

namespace cgen::target {

class CodeWriter {
public:
  explicit CodeWriter(std::ostream &out, int width);
  ~CodeWriter();

  CodeWriter(const CodeWriter &) = delete;
  auto operator=(const CodeWriter &) -> CodeWriter & = delete;

  CodeWriter(CodeWriter &&) = default;
  auto operator=(CodeWriter &&) -> CodeWriter & = default;

  void write(const Expr &expr);

private:
  std::ostream &out_;
  int width_;
};

} // namespace cgen::target
