#pragma once
#include <ranges>

namespace cgen::target::common::concepts {
template <typename R, typename T>
concept RangeOf = std::ranges::input_range<R> &&
                  std::constructible_from<T, std::ranges::range_reference_t<R>>;
} // namespace cgen::target::common::concepts
