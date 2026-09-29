#include "sea_port/crane_optimizer.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "sea_port/port_simulation.h"

namespace sea_port {

OptimizationReport CraneOptimizer::Optimize(const SimulationConfig& base_config,
                                            const OptimizationOptions& options) {
  if (options.trials_per_configuration < 1) {
    throw std::invalid_argument("Optimizer needs at least one trial");
  }
  for (int maximum : options.maximum_cranes) {
    if (maximum < 1) {
      throw std::invalid_argument("Every maximum crane count must be positive");
    }
  }

  OptimizationReport report;
  double best_profit = -std::numeric_limits<double>::infinity();
  double best_unrestricted_profit = -std::numeric_limits<double>::infinity();
  std::optional<OptimizationResult> best_unrestricted_result;
  for (int bulk = 1; bulk <= options.maximum_cranes[0]; ++bulk) {
    for (int liquid = 1; liquid <= options.maximum_cranes[1]; ++liquid) {
      for (int container = 1; container <= options.maximum_cranes[2]; ++container) {
        OptimizationResult result;
        result.crane_counts = {bulk, liquid, container};
        double rating_sum = 0.0;
        double service_rate_sum = 0.0;

        for (int trial = 0; trial < options.trials_per_configuration; ++trial) {
          SimulationConfig config = base_config;
          config.crane_counts = result.crane_counts;
          // Every candidate sees the same seeds and therefore the same external scenario.
          config.random_seed = base_config.random_seed + static_cast<std::uint64_t>(trial);
          PortSimulation simulation(config);
          simulation.RunRandomMonth();
          result.average_operating_profit += simulation.statistics().net_revenue();
          rating_sum += simulation.rating();
          service_rate_sum += simulation.statistics().service_rate();
          if (trial == 0) {
            result.purchase_cost = simulation.CranePurchaseCost();
          }
        }

        const double trials = static_cast<double>(options.trials_per_configuration);
        result.average_operating_profit /= trials;
        result.average_rating = rating_sum / trials;
        result.average_service_rate = service_rate_sum / trials;
        result.average_investment_profit = result.average_operating_profit - result.purchase_cost;
        result.payback_months = result.average_operating_profit > 0.0
                                    ? result.purchase_cost / result.average_operating_profit
                                    : std::numeric_limits<double>::infinity();
        result.meets_quality_limits =
            result.average_rating >= base_config.minimum_rating_for_optimization &&
            result.average_service_rate >= base_config.minimum_service_rate_for_optimization;

        if (result.meets_quality_limits && result.average_investment_profit > best_profit) {
          best_profit = result.average_investment_profit;
          report.best_result = result;
        }
        if (result.average_investment_profit > best_unrestricted_profit) {
          best_unrestricted_profit = result.average_investment_profit;
          best_unrestricted_result = result;
        }
        report.results.push_back(result);
      }
    }
  }

  std::sort(report.results.begin(), report.results.end(),
            [](const OptimizationResult& left, const OptimizationResult& right) {
              return left.average_investment_profit > right.average_investment_profit;
            });
  if (!report.best_result.has_value()) {
    report.best_result = best_unrestricted_result;
  }
  return report;
}

}  // namespace sea_port
