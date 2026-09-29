#ifndef SEA_PORT_CRANE_OPTIMIZER_H_
#define SEA_PORT_CRANE_OPTIMIZER_H_

#include <array>
#include <optional>
#include <vector>

#include "sea_port/types.h"

namespace sea_port {

struct OptimizationOptions {
  std::array<int, kCargoTypeCount> maximum_cranes = {4, 3, 3};
  int trials_per_configuration = 5;
};

struct OptimizationResult {
  std::array<int, kCargoTypeCount> crane_counts = {0, 0, 0};
  double purchase_cost = 0.0;
  double average_operating_profit = 0.0;
  double average_investment_profit = 0.0;
  double average_rating = 0.0;
  double average_service_rate = 0.0;
  double payback_months = 0.0;
  bool meets_quality_limits = false;
};

struct OptimizationReport {
  std::vector<OptimizationResult> results;
  std::optional<OptimizationResult> best_result;
};

class CraneOptimizer {
 public:
  static OptimizationReport Optimize(const SimulationConfig& base_config,
                                     const OptimizationOptions& options = {});
};

}  // namespace sea_port

#endif  // SEA_PORT_CRANE_OPTIMIZER_H_
