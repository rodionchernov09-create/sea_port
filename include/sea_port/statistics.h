#ifndef SEA_PORT_STATISTICS_H_
#define SEA_PORT_STATISTICS_H_

#include <cstddef>
#include <vector>

#include "sea_port/types.h"

namespace sea_port {

class Statistics {
 public:
  void RecordArrival();
  void RecordRejection();
  void RecordAbandonment();
  void RecordQueueLength(std::size_t length);
  void RecordCompletion(const CompletionRecord& record);
  void set_unfinished_ships(std::size_t count);

  int arrived_ships() const;
  int completed_ships() const;
  int abandoned_ships() const;
  int rejected_ships() const;
  std::size_t unfinished_ships() const;
  double average_queue_length() const;
  std::size_t maximum_queue_length() const;
  double average_waiting_days() const;
  int maximum_waiting_days() const;
  double average_unloading_delay_days() const;
  int maximum_unloading_delay_days() const;
  double gross_revenue() const;
  double total_penalties() const;
  double net_revenue() const;
  double service_rate() const;
  const std::vector<CompletionRecord>& completions() const;

 private:
  int arrived_ships_ = 0;
  int abandoned_ships_ = 0;
  int rejected_ships_ = 0;
  std::size_t unfinished_ships_ = 0;
  std::size_t queue_samples_ = 0;
  std::size_t queue_length_sum_ = 0;
  std::size_t maximum_queue_length_ = 0;
  int waiting_days_sum_ = 0;
  int maximum_waiting_days_ = 0;
  int unloading_delay_sum_ = 0;
  int maximum_unloading_delay_days_ = 0;
  double gross_revenue_ = 0.0;
  double total_penalties_ = 0.0;
  double net_revenue_ = 0.0;
  std::vector<CompletionRecord> completions_;
};

}  // namespace sea_port

#endif  // SEA_PORT_STATISTICS_H_
