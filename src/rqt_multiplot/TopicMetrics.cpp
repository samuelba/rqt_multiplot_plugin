/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include "rqt_multiplot/TopicMetrics.hpp"

namespace rqt_multiplot {

namespace {

constexpr double kNsPerSecond = 1e9;

struct MetricInfo {
  TopicMetric metric;
  const char* name;
  const char* label;
};

constexpr std::array<MetricInfo, kTopicMetricCount> kMetricInfos{{
    {TopicMetric::Rate, "rate", "Rate [Hz]"},
    {TopicMetric::PeriodMin, "period_min", "Period min [s]"},
    {TopicMetric::PeriodMax, "period_max", "Period max [s]"},
    {TopicMetric::PeriodStdDev, "period_std_dev", "Period std dev [s]"},
    {TopicMetric::Bandwidth, "bandwidth", "Bandwidth [B/s]"},
    {TopicMetric::SizeMean, "size_mean", "Message size mean [B]"},
    {TopicMetric::SizeMin, "size_min", "Message size min [B]"},
    {TopicMetric::SizeMax, "size_max", "Message size max [B]"},
    {TopicMetric::DelayMean, "delay_mean", "Delay mean [s]"},
    {TopicMetric::DelayMin, "delay_min", "Delay min [s]"},
    {TopicMetric::DelayMax, "delay_max", "Delay max [s]"},
    {TopicMetric::DelayStdDev, "delay_std_dev", "Delay std dev [s]"},
}};

struct Statistics {
  double mean = 0.0;
  double min = 0.0;
  double max = 0.0;
  double stdDev = 0.0;
};

template <typename T>
std::optional<Statistics> statisticsOf(const std::vector<T>& values) {
  if (values.empty()) {
    return std::nullopt;
  }
  const auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
  double sum = 0.0;
  for (const auto value : values) {
    sum += static_cast<double>(value);
  }
  const auto count = static_cast<double>(values.size());
  Statistics statistics;
  statistics.mean = sum / count;
  statistics.min = static_cast<double>(*minIt);
  statistics.max = static_cast<double>(*maxIt);
  double squares = 0.0;
  for (const auto value : values) {
    const double deviation = static_cast<double>(value) - statistics.mean;
    squares += deviation * deviation;
  }
  statistics.stdDev = std::sqrt(squares / count);
  return statistics;
}

double pick(const Statistics& statistics, TopicMetric metric) {
  switch (metric) {
    case TopicMetric::PeriodMin:
    case TopicMetric::SizeMin:
    case TopicMetric::DelayMin:
      return statistics.min;
    case TopicMetric::PeriodMax:
    case TopicMetric::SizeMax:
    case TopicMetric::DelayMax:
      return statistics.max;
    case TopicMetric::PeriodStdDev:
    case TopicMetric::DelayStdDev:
      return statistics.stdDev;
    default:
      return statistics.mean;
  }
}

}  // namespace

const char* topicMetricName(TopicMetric metric) {
  return kMetricInfos.at(static_cast<size_t>(metric)).name;
}

std::optional<TopicMetric> topicMetricFromName(const std::string& name) {
  for (const auto& info : kMetricInfos) {
    if (name == info.name) {
      return info.metric;
    }
  }
  return std::nullopt;
}

const char* topicMetricLabel(TopicMetric metric) {
  return kMetricInfos.at(static_cast<size_t>(metric)).label;
}

bool isDelayMetric(TopicMetric metric) {
  return metric >= TopicMetric::DelayMean;
}

TopicMetricsWindow::TopicMetricsWindow(size_t windowSize) : windowSize_(std::clamp(windowSize, kMinWindowSize, kMaxWindowSize)) {}

void TopicMetricsWindow::setWindowSize(size_t windowSize) {
  windowSize_ = std::clamp(windowSize, kMinWindowSize, kMaxWindowSize);
  while (samples_.size() > windowSize_) {
    samples_.pop_front();
  }
}

size_t TopicMetricsWindow::getWindowSize() const {
  return windowSize_;
}

size_t TopicMetricsWindow::size() const {
  return samples_.size();
}

void TopicMetricsWindow::addSample(int64_t receiptTimeNs, size_t bytes, std::optional<int64_t> stampNs) {
  std::optional<int64_t> delayNs;
  if (stampNs) {
    delayNs = receiptTimeNs - *stampNs;
  }
  samples_.push_back({receiptTimeNs, bytes, delayNs});
  if (samples_.size() > windowSize_) {
    samples_.pop_front();
  }
}

void TopicMetricsWindow::reset() {
  samples_.clear();
}

std::optional<double> TopicMetricsWindow::value(TopicMetric metric) const {
  switch (metric) {
    case TopicMetric::Rate:
    case TopicMetric::PeriodMin:
    case TopicMetric::PeriodMax:
    case TopicMetric::PeriodStdDev:
      return periodValue(metric);
    case TopicMetric::Bandwidth:
    case TopicMetric::SizeMean:
    case TopicMetric::SizeMin:
    case TopicMetric::SizeMax:
      return sizeValue(metric);
    default:
      return delayValue(metric);
  }
}

std::optional<double> TopicMetricsWindow::periodValue(TopicMetric metric) const {
  if (samples_.size() < 2) {
    return std::nullopt;
  }
  const int64_t spanNs = samples_.back().receiptTimeNs - samples_.front().receiptTimeNs;
  if (metric == TopicMetric::Rate) {
    if (spanNs <= 0) {
      return std::nullopt;
    }
    return static_cast<double>(samples_.size() - 1) / (static_cast<double>(spanNs) / kNsPerSecond);
  }
  std::vector<int64_t> periodsNs;
  periodsNs.reserve(samples_.size() - 1);
  for (size_t i = 1; i < samples_.size(); ++i) {
    periodsNs.push_back(samples_[i].receiptTimeNs - samples_[i - 1].receiptTimeNs);
  }
  return pick(*statisticsOf(periodsNs), metric) / kNsPerSecond;
}

std::optional<double> TopicMetricsWindow::sizeValue(TopicMetric metric) const {
  if (samples_.empty() || samples_.back().bytes == 0) {
    return std::nullopt;
  }
  if (metric == TopicMetric::Bandwidth) {
    const int64_t spanNs = samples_.back().receiptTimeNs - samples_.front().receiptTimeNs;
    if (samples_.size() < 2 || spanNs <= 0) {
      return std::nullopt;
    }
    size_t bytes = 0;
    for (size_t i = 1; i < samples_.size(); ++i) {
      bytes += samples_[i].bytes;
    }
    return static_cast<double>(bytes) / (static_cast<double>(spanNs) / kNsPerSecond);
  }
  std::vector<size_t> sizes;
  sizes.reserve(samples_.size());
  for (const auto& sample : samples_) {
    if (sample.bytes > 0) {
      sizes.push_back(sample.bytes);
    }
  }
  return pick(*statisticsOf(sizes), metric);
}

std::optional<double> TopicMetricsWindow::delayValue(TopicMetric metric) const {
  std::vector<int64_t> delaysNs;
  for (const auto& sample : samples_) {
    if (sample.delayNs) {
      delaysNs.push_back(*sample.delayNs);
    }
  }
  const auto statistics = statisticsOf(delaysNs);
  if (!statistics) {
    return std::nullopt;
  }
  return pick(*statistics, metric) / kNsPerSecond;
}

}  // namespace rqt_multiplot
