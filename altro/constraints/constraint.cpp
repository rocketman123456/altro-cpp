// Copyright [2021] Optimus Ride Inc.

#include "altro/constraints/constraint.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>

namespace altro {
namespace constraints {

std::string ConstraintInfo::ToString(int precision) const {
  Eigen::IOFormat format(precision, 0, ", ", "", "", "", "[", "]");
  return fmt::format("{} at index {}: {}", label, index, fmt::streamed(violation.format(format)));
}

std::ostream& operator<<(std::ostream& os, const ConstraintInfo& coninfo) {
  return os << coninfo.ToString();
}

}  // namespace constraints
}  // namespace altro