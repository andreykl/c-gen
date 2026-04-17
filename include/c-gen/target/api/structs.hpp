#pragma once
#include <c-gen/target/core/ast.hpp>

namespace cgen::target::api {
using namespace core;

struct Operation {
  explicit Operation(AccessKey, Assignment value) : value(std::move(value)) {}
  Assignment value;
};

struct InitField {
  explicit InitField(AccessKey, Assignment value) : value(std::move(value)) {}
  Assignment value;
};

struct Port {
  explicit Port(AccessKey, ListInit value) : value(std::move(value)) {}
  ListInit value;
};

struct File {
  explicit File(AccessKey, core::File value) : value(std::move(value)) {}
  core::File value;
};

} // namespace cgen::target::api
