#include "sea_port/statistics.h"

#include <algorithm>

namespace sea_port {

void Statistics::RecordArrival() { ++arrived_ships_; }
void Statistics::RecordRejection() { ++rejected_ships_; }
void Statistics::RecordAbandonment() { ++abandoned_ships_; }

void Statistics::RecordQueueLength(std::size_t length) {
  ++queue_samples_;
  queue_length_sum_ += length;
  maximum_queue_length_ = std::max(maximum_queue_length_, length);
}

void Statistics::RecordCompletion(const CompletionRecord& record) {
  completions_.push_back(record);
  waiting_days_sum_ += record.waiting_days;
  maximum_waiting_days_ = std::max(maximum_waiting_days_, record.waiting_days);
  unloading_delay_sum_ += record.additional_unloading_delay_days;
  maximum_unloading_delay_days_ =
      std::max(maximum_unloading_delay_days_, record.additional_unloading_delay_days);
  gross_revenue_ += record.credited_revenue;
  total_penalties_ += record.penalty;
  net_revenue_ += record.net_revenue;
}

void Statistics::set_unfinished_ships(std::size_t count) { unfinished_ships_ = count; }

int Statistics::arrived_ships() const { return arrived_ships_; }
int Statistics::completed_ships() const { return static_cast<int>(completions_.size()); }
int Statistics::abandoned_ships() const { return abandoned_ships_; }
int Statistics::rejected_ships() const { return rejected_ships_; }
std::size_t Statistics::unfinished_ships() const { return unfinished_ships_; }

double Statistics::average_queue_length() const {
  return queue_samples_ == 0
             ? 0.0
             : static_cast<double>(queue_length_sum_) / static_cast<double>(queue_samples_);
}

std::size_t Statistics::maximum_queue_length() const { return maximum_queue_length_; }

double Statistics::average_waiting_days() const {
  return completions_.empty()
             ? 0.0
             : static_cast<double>(waiting_days_sum_) / static_cast<double>(completions_.size());
}

int Statistics::maximum_waiting_days() const { return maximum_waiting_days_; }

double Statistics::average_unloading_delay_days() const {
  return completions_.empty()
             ? 0.0
             : static_cast<double>(unloading_delay_sum_) / static_cast<double>(completions_.size());
}

int Statistics::maximum_unloading_delay_days() const { return maximum_unloading_delay_days_; }

double Statistics::gross_revenue() const { return gross_revenue_; }
double Statistics::total_penalties() const { return total_penalties_; }
double Statistics::net_revenue() const { return net_revenue_; }

double Statistics::service_rate() const {
  return arrived_ships_ == 0
             ? 1.0
             : static_cast<double>(completed_ships()) / static_cast<double>(arrived_ships_);
}

const std::vector<CompletionRecord>& Statistics::completions() const { return completions_; }

}  // namespace sea_port
