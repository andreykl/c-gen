#pragma once

#include <c-gen/target/api/iwriter.hpp>
#include <c-gen/target/core/ast.hpp>
#include <iostream>

namespace cgen::target {
using namespace api;

/**
 * @brief Concrete implementation of IWriter for C code generation.
 *
 * This class handles the transformation of AST (Expr) into pretty-printed C
 * code. It follows the Rule of Five: non-copyable, but movable.
 */
class CodeWriter : public IWriter {
public:
  /**
   * @brief Constructs a writer bound to an existing output stream.
   * @param out The output stream (e.g., std::cout or an std::ofstream).
   * @param width The maximum line width for the pretty-printer.
   */
  explicit CodeWriter(std::ostream &out, int width) : out(&out), width(width) {}

  /**
   * @brief Virtual destructor.
   */
  ~CodeWriter() override = default;

  // --- Rule of Five (Safety first) ---

  // Copying is disabled because we shouldn't duplicate ownership of the stream.
  CodeWriter(const CodeWriter &) = delete;
  auto operator=(const CodeWriter &) -> CodeWriter & = delete;

  /**
   * @brief Move constructor.
   * Transfers stream ownership and nulls out the source.
   */
  CodeWriter(CodeWriter &&other) noexcept : out(other.out), width(other.width) {
    other.out = nullptr;
  }

  /**
   * @brief Move assignment operator.
   */
  auto operator=(CodeWriter &&other) noexcept -> CodeWriter & {
    if (this != &other) {
      out = other.out;
      width = other.width;
      other.out = nullptr;
    }
    return *this;
  }

  /**
   * @brief Renders the AST expression to the output stream.
   * Implements the IWriter interface.
   *
   * @param expr The AST root or node to be printed.
   */
  void write(const api::File &file) const override;

private:
  std::ostream *out;
  int width;
};

} // namespace cgen::target
