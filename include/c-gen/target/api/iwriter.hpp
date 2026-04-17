#pragma once
#include <c-gen/target/api/structs.hpp>

namespace cgen::target::api {

/**
 * @brief Abstract interface for any code output format.
 */
class IWriter {
public:
  virtual ~IWriter() = default;

  /**
   * @brief Writes the AST expression to the target.
   * Const-qualified to support temporary writer objects.
   */
  virtual void write(const File &file) const = 0;
};

} // namespace cgen::target::api
