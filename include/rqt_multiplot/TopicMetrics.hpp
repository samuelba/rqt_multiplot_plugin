/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>

namespace rqt_multiplot {

// Same semantics as ros2 topic hz / bw / delay. Period and delay statistics are in seconds.
enum class TopicMetric {
  Rate,
  PeriodMin,
  PeriodMax,
  PeriodStdDev,
  Bandwidth,
  SizeMean,
  SizeMin,
  SizeMax,
  DelayMean,
  DelayMin,
  DelayMax,
  DelayStdDev
};

constexpr int kTopicMetricCount = static_cast<int>(TopicMetric::DelayStdDev) + 1;

const char* topicMetricName(TopicMetric metric);
std::optional<TopicMetric> topicMetricFromName(const std::string& name);
const char* topicMetricLabel(TopicMetric metric);
bool isDelayMetric(TopicMetric metric);

class TopicMetricsWindow {
 public:
  static constexpr size_t kDefaultWindowSize = 100;
  static constexpr size_t kMinWindowSize = 2;
  static constexpr size_t kMaxWindowSize = 10000;

  explicit TopicMetricsWindow(size_t windowSize = kDefaultWindowSize);

  void setWindowSize(size_t windowSize);
  size_t getWindowSize() const;
  size_t size() const;

  // bytes = 0 means the serialized size is unknown.
  void addSample(int64_t receiptTimeNs, size_t bytes, std::optional<int64_t> stampNs);
  std::optional<double> value(TopicMetric metric) const;
  void reset();

 private:
  struct Sample {
    int64_t receiptTimeNs;
    size_t bytes;
    std::optional<int64_t> delayNs;
  };

  std::deque<Sample> samples_;
  size_t windowSize_;

  std::optional<double> periodValue(TopicMetric metric) const;
  std::optional<double> sizeValue(TopicMetric metric) const;
  std::optional<double> delayValue(TopicMetric metric) const;
};

}  // namespace rqt_multiplot
