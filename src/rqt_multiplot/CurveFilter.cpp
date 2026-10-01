/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CurveFilter.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>

namespace rqt_multiplot {

namespace {

constexpr size_t kWindowResyncInterval = 1024;

struct FilterTypeInfo {
  CurveFilterType type;
  const char* key;
  const char* name;
  bool hasParameters;
  const char* description;
};

constexpr std::array<FilterTypeInfo, 12> kFilterTypes{{
    {CurveFilterType::Derivative, "derivative", "Derivative", true,
     "Rate of change dy/dx. Uses the time step between points, or a fixed step. Drops the first point."},
    {CurveFilterType::Integral, "integral", "Integral", true,
     "Running integral of y over x (trapezoid rule). Starts at 0. Uses the time step between points, or a fixed step."},
    {CurveFilterType::MovingAverage, "moving_average", "Moving average", true,
     "Mean of the last N samples. Lag compensation moves each point to the center of its window."},
    {CurveFilterType::MovingRms, "moving_rms", "Moving RMS", true, "Root mean square of the last N samples."},
    {CurveFilterType::MovingVariance, "moving_variance", "Moving variance", true,
     "Variance of the last N samples, or the standard deviation if selected."},
    {CurveFilterType::LowPass, "low_pass", "Low-pass", true,
     "First-order low-pass filter. A larger time constant gives a smoother curve with more lag."},
    {CurveFilterType::Absolute, "absolute", "Absolute", false, "Absolute value |y|."},
    {CurveFilterType::ScaleOffset, "scale_offset", "Scale/offset", true, "y * scale + offset."},
    {CurveFilterType::Threshold, "threshold", "Threshold", true, "1 if the condition is true, else 0."},
    {CurveFilterType::OutlierRemoval, "outlier_removal", "Outlier removal", true,
     "Removes one-sample spikes: a jump in and back out that is larger than the factor times the step before it. "
     "Adds a delay of one sample."},
    {CurveFilterType::TimeSincePrevious, "time_since_previous", "Time since previous point", false,
     "Time from the previous point to this point (dx). Shows gaps and changes in the message rate."},
    {CurveFilterType::SamplesCount, "samples_count", "Samples count", true,
     "Number of samples in the last T milliseconds, this point included. Shows the message rate."},
}};

const FilterTypeInfo& typeInfo(CurveFilterType type) {
  for (const auto& info : kFilterTypes) {
    if (info.type == type) {
      return info;
    }
  }
  return kFilterTypes[0];
}

QString comparisonSymbol(CurveFilterComparison comparison) {
  switch (comparison) {
    case CurveFilterComparison::Equal:
      return QStringLiteral("=");
    case CurveFilterComparison::Less:
      return QStringLiteral("<");
    case CurveFilterComparison::LessEqual:
      return QStringLiteral("<=");
    case CurveFilterComparison::Greater:
      return QStringLiteral(">");
    case CurveFilterComparison::GreaterEqual:
      return QStringLiteral(">=");
    case CurveFilterComparison::Range:
    default:
      return QStringLiteral("in");
  }
}

size_t windowCapacity(int windowSize) {
  return static_cast<size_t>(std::max(windowSize, 1));
}

class SlidingWindow {
 public:
  explicit SlidingWindow(size_t capacity) : capacity_(capacity) {}

  void push(const QPointF& point) {
    points_.push_back(point);
    add(point.y());
    if (points_.size() > capacity_) {
      remove(points_.front().y());
      points_.pop_front();
    }
    if (++pushesSinceResync_ >= kWindowResyncInterval) {
      resync();
    }
  }

  void clear() {
    points_.clear();
    mean_ = 0.0;
    m2_ = 0.0;
    sumSquares_ = 0.0;
    pushesSinceResync_ = 0;
  }

  double mean() const { return mean_; }
  double variance() const { return points_.empty() ? 0.0 : std::max(m2_, 0.0) / static_cast<double>(points_.size()); }
  double rms() const { return points_.empty() ? 0.0 : std::sqrt(std::max(sumSquares_, 0.0) / static_cast<double>(points_.size())); }
  double centerX() const { return 0.5 * (points_.front().x() + points_.back().x()); }

 private:
  void add(double value) {
    const auto count = static_cast<double>(points_.size());
    const double delta = value - mean_;
    mean_ += delta / count;
    m2_ += delta * (value - mean_);
    sumSquares_ += value * value;
  }

  void remove(double value) {
    const auto count = static_cast<double>(points_.size() - 1);
    const double delta = value - mean_;
    mean_ -= delta / count;
    m2_ -= delta * (value - mean_);
    sumSquares_ -= value * value;
  }

  void resync() {
    pushesSinceResync_ = 0;
    double sum = 0.0;
    double sumSquares = 0.0;
    for (const auto& point : points_) {
      sum += point.y();
      sumSquares += point.y() * point.y();
    }
    const auto count = static_cast<double>(points_.size());
    mean_ = sum / count;
    double m2 = 0.0;
    for (const auto& point : points_) {
      m2 += (point.y() - mean_) * (point.y() - mean_);
    }
    m2_ = m2;
    sumSquares_ = sumSquares;
  }

  size_t capacity_;
  std::deque<QPointF> points_;
  double mean_ = 0.0;
  double m2_ = 0.0;
  double sumSquares_ = 0.0;
  size_t pushesSinceResync_ = 0;
};

class StepFilter : public CurveFilter {
 public:
  explicit StepFilter(const CurveFilterSpec& spec) : useFixedStep_(spec.useFixedStep), fixedStep_(spec.fixedStep) {}

  void reset() override { previous_.reset(); }

 protected:
  std::optional<double> step(const QPointF& point) const {
    if (!previous_.has_value()) {
      return std::nullopt;
    }
    const double dt = useFixedStep_ ? fixedStep_ : point.x() - previous_->x();
    if (dt <= 0.0) {
      return std::nullopt;
    }
    return dt;
  }

  bool hasPrevious() const { return previous_.has_value(); }
  const QPointF& previous() const { return *previous_; }
  void setPrevious(const QPointF& point) { previous_ = point; }

 private:
  bool useFixedStep_;
  double fixedStep_;
  std::optional<QPointF> previous_;
};

class DerivativeFilter : public StepFilter {
 public:
  using StepFilter::StepFilter;

  std::optional<QPointF> process(const QPointF& point) override {
    if (!hasPrevious()) {
      setPrevious(point);
      return std::nullopt;
    }
    const std::optional<double> dt = step(point);
    if (!dt.has_value()) {
      return std::nullopt;
    }
    const QPointF derivative(point.x(), (point.y() - previous().y()) / *dt);
    setPrevious(point);
    return derivative;
  }
};

class IntegralFilter : public StepFilter {
 public:
  using StepFilter::StepFilter;

  std::optional<QPointF> process(const QPointF& point) override {
    if (!hasPrevious()) {
      setPrevious(point);
      return QPointF(point.x(), total_);
    }
    const std::optional<double> dt = step(point);
    if (!dt.has_value()) {
      return std::nullopt;
    }
    total_ += 0.5 * (point.y() + previous().y()) * *dt;
    setPrevious(point);
    return QPointF(point.x(), total_);
  }

  void reset() override {
    StepFilter::reset();
    total_ = 0.0;
  }

 private:
  double total_ = 0.0;
};

class MovingAverageFilter : public CurveFilter {
 public:
  explicit MovingAverageFilter(const CurveFilterSpec& spec)
      : window_(windowCapacity(spec.windowSize)), compensateLag_(spec.compensateLag) {}

  std::optional<QPointF> process(const QPointF& point) override {
    window_.push(point);
    return QPointF(compensateLag_ ? window_.centerX() : point.x(), window_.mean());
  }

  void reset() override { window_.clear(); }

 private:
  SlidingWindow window_;
  bool compensateLag_;
};

class MovingRmsFilter : public CurveFilter {
 public:
  explicit MovingRmsFilter(const CurveFilterSpec& spec) : window_(windowCapacity(spec.windowSize)) {}

  std::optional<QPointF> process(const QPointF& point) override {
    window_.push(point);
    return QPointF(point.x(), window_.rms());
  }

  void reset() override { window_.clear(); }

 private:
  SlidingWindow window_;
};

class MovingVarianceFilter : public CurveFilter {
 public:
  explicit MovingVarianceFilter(const CurveFilterSpec& spec)
      : window_(windowCapacity(spec.windowSize)), standardDeviation_(spec.standardDeviation) {}

  std::optional<QPointF> process(const QPointF& point) override {
    window_.push(point);
    const double variance = window_.variance();
    return QPointF(point.x(), standardDeviation_ ? std::sqrt(variance) : variance);
  }

  void reset() override { window_.clear(); }

 private:
  SlidingWindow window_;
  bool standardDeviation_;
};

class LowPassFilter : public CurveFilter {
 public:
  explicit LowPassFilter(const CurveFilterSpec& spec) : timeConstant_(spec.timeConstant) {}

  std::optional<QPointF> process(const QPointF& point) override {
    if (!state_.has_value() || timeConstant_ <= 0.0) {
      state_ = point;
      return point;
    }
    const double dt = point.x() - state_->x();
    if (dt <= 0.0) {
      return std::nullopt;
    }
    const double alpha = dt / (timeConstant_ + dt);
    state_ = QPointF(point.x(), state_->y() + alpha * (point.y() - state_->y()));
    return state_;
  }

  void reset() override { state_.reset(); }

 private:
  double timeConstant_;
  std::optional<QPointF> state_;
};

class AbsoluteFilter : public CurveFilter {
 public:
  std::optional<QPointF> process(const QPointF& point) override { return QPointF(point.x(), std::abs(point.y())); }
  void reset() override {}
};

class ScaleOffsetFilter : public CurveFilter {
 public:
  explicit ScaleOffsetFilter(const CurveFilterSpec& spec) : scale_(spec.scale), offset_(spec.offset) {}

  std::optional<QPointF> process(const QPointF& point) override { return QPointF(point.x(), point.y() * scale_ + offset_); }
  void reset() override {}

 private:
  double scale_;
  double offset_;
};

class ThresholdFilter : public CurveFilter {
 public:
  explicit ThresholdFilter(const CurveFilterSpec& spec) : comparison_(spec.comparison), a_(spec.thresholdA), b_(spec.thresholdB) {}

  std::optional<QPointF> process(const QPointF& point) override { return QPointF(point.x(), matches(point.y()) ? 1.0 : 0.0); }
  void reset() override {}

 private:
  bool matches(double value) const {
    switch (comparison_) {
      case CurveFilterComparison::Equal:
        return value == a_;
      case CurveFilterComparison::Less:
        return value < a_;
      case CurveFilterComparison::LessEqual:
        return value <= a_;
      case CurveFilterComparison::Greater:
        return value > a_;
      case CurveFilterComparison::GreaterEqual:
        return value >= a_;
      case CurveFilterComparison::Range:
      default:
        return value >= std::min(a_, b_) && value <= std::max(a_, b_);
    }
  }

  CurveFilterComparison comparison_;
  double a_;
  double b_;
};

// Emits each point one sample late, so the candidate can be compared with both neighbours.
class OutlierRemovalFilter : public CurveFilter {
 public:
  explicit OutlierRemovalFilter(const CurveFilterSpec& spec) : factor_(spec.outlierFactor) {}

  std::optional<QPointF> process(const QPointF& point) override {
    history_.push_back(point);
    if (history_.size() > kHistorySize) {
      history_.pop_front();
    }
    if (history_.size() < 2) {
      return std::nullopt;
    }
    const QPointF candidate = history_[history_.size() - 2];
    if (history_.size() == kHistorySize && isSpike()) {
      history_.erase(history_.end() - 2);
      return std::nullopt;
    }
    return candidate;
  }

  void reset() override { history_.clear(); }

 private:
  static constexpr size_t kHistorySize = 4;

  bool isSpike() const {
    const double before = history_[1].y() - history_[0].y();
    const double into = history_[2].y() - history_[1].y();
    const double out = history_[3].y() - history_[2].y();
    if (into * out >= 0.0) {
      return false;
    }
    const double jump = std::max(std::abs(into), std::abs(out));
    return jump > factor_ * std::abs(before);
  }

  double factor_;
  std::deque<QPointF> history_;
};

class TimeSincePreviousFilter : public CurveFilter {
 public:
  std::optional<QPointF> process(const QPointF& point) override {
    const std::optional<QPointF> previous = previous_;
    previous_ = point;
    if (!previous.has_value()) {
      return std::nullopt;
    }
    return QPointF(point.x(), point.x() - previous->x());
  }

  void reset() override { previous_.reset(); }

 private:
  std::optional<QPointF> previous_;
};

class SamplesCountFilter : public CurveFilter {
 public:
  explicit SamplesCountFilter(const CurveFilterSpec& spec) : window_(0.001 * static_cast<double>(std::max(spec.windowMilliseconds, 1))) {}

  std::optional<QPointF> process(const QPointF& point) override {
    times_.push_back(point.x());
    while (!times_.empty() && times_.front() <= point.x() - window_) {
      times_.pop_front();
    }
    return QPointF(point.x(), static_cast<double>(times_.size()));
  }

  void reset() override { times_.clear(); }

 private:
  double window_;
  std::deque<double> times_;
};

}  // namespace

bool CurveFilterSpec::operator==(const CurveFilterSpec& other) const {
  return type == other.type && useFixedStep == other.useFixedStep && fixedStep == other.fixedStep && windowSize == other.windowSize &&
         compensateLag == other.compensateLag && standardDeviation == other.standardDeviation && timeConstant == other.timeConstant &&
         scale == other.scale && offset == other.offset && comparison == other.comparison && thresholdA == other.thresholdA &&
         thresholdB == other.thresholdB && outlierFactor == other.outlierFactor && windowMilliseconds == other.windowMilliseconds;
}

QVector<CurveFilterType> allCurveFilterTypes() {
  QVector<CurveFilterType> types;
  for (const auto& info : kFilterTypes) {
    types.append(info.type);
  }
  return types;
}

QString curveFilterTypeName(CurveFilterType type) {
  return QString::fromLatin1(typeInfo(type).name);
}

QString curveFilterTypeDescription(CurveFilterType type) {
  return QString::fromLatin1(typeInfo(type).description);
}

QString curveFilterTypeKey(CurveFilterType type) {
  return QString::fromLatin1(typeInfo(type).key);
}

std::optional<CurveFilterType> curveFilterTypeFromKey(const QString& key) {
  for (const auto& info : kFilterTypes) {
    if (key == QLatin1String(info.key)) {
      return info.type;
    }
  }
  return std::nullopt;
}

bool curveFilterHasParameters(CurveFilterType type) {
  return typeInfo(type).hasParameters;
}

QString curveFilterSummary(const CurveFilterSpec& spec) {
  const QString name = curveFilterTypeName(spec.type);
  switch (spec.type) {
    case CurveFilterType::Derivative:
    case CurveFilterType::Integral:
      return spec.useFixedStep ? QStringLiteral("%1 (dt %2)").arg(name).arg(spec.fixedStep) : name;
    case CurveFilterType::MovingAverage:
    case CurveFilterType::MovingRms:
      return QStringLiteral("%1 (%2)").arg(name).arg(spec.windowSize);
    case CurveFilterType::MovingVariance:
      return QStringLiteral("%1 (%2)").arg(spec.standardDeviation ? QStringLiteral("Moving std dev") : name).arg(spec.windowSize);
    case CurveFilterType::LowPass:
      return QStringLiteral("%1 (tau %2 s)").arg(name).arg(spec.timeConstant);
    case CurveFilterType::ScaleOffset:
      return QStringLiteral("%1 (y * %2 + %3)").arg(name).arg(spec.scale).arg(spec.offset);
    case CurveFilterType::Threshold:
      if (spec.comparison == CurveFilterComparison::Range) {
        return QStringLiteral("%1 (in [%2, %3])").arg(name).arg(spec.thresholdA).arg(spec.thresholdB);
      }
      return QStringLiteral("%1 (%2 %3)").arg(name, comparisonSymbol(spec.comparison)).arg(spec.thresholdA);
    case CurveFilterType::OutlierRemoval:
      return QStringLiteral("%1 (%2)").arg(name).arg(spec.outlierFactor);
    case CurveFilterType::SamplesCount:
      return QStringLiteral("%1 (%2 ms)").arg(name).arg(spec.windowMilliseconds);
    case CurveFilterType::Absolute:
    case CurveFilterType::TimeSincePrevious:
    default:
      return name;
  }
}

CurveFilterSpec defaultCurveFilterSpec(CurveFilterType type) {
  CurveFilterSpec spec;
  spec.type = type;
  return spec;
}

std::unique_ptr<CurveFilter> createCurveFilter(const CurveFilterSpec& spec) {
  switch (spec.type) {
    case CurveFilterType::Derivative:
      return std::make_unique<DerivativeFilter>(spec);
    case CurveFilterType::Integral:
      return std::make_unique<IntegralFilter>(spec);
    case CurveFilterType::MovingAverage:
      return std::make_unique<MovingAverageFilter>(spec);
    case CurveFilterType::MovingRms:
      return std::make_unique<MovingRmsFilter>(spec);
    case CurveFilterType::MovingVariance:
      return std::make_unique<MovingVarianceFilter>(spec);
    case CurveFilterType::LowPass:
      return std::make_unique<LowPassFilter>(spec);
    case CurveFilterType::Absolute:
      return std::make_unique<AbsoluteFilter>();
    case CurveFilterType::ScaleOffset:
      return std::make_unique<ScaleOffsetFilter>(spec);
    case CurveFilterType::Threshold:
      return std::make_unique<ThresholdFilter>(spec);
    case CurveFilterType::OutlierRemoval:
      return std::make_unique<OutlierRemovalFilter>(spec);
    case CurveFilterType::TimeSincePrevious:
      return std::make_unique<TimeSincePreviousFilter>();
    case CurveFilterType::SamplesCount:
      return std::make_unique<SamplesCountFilter>(spec);
  }
  return nullptr;
}

CurveFilterChain::CurveFilterChain(const QVector<CurveFilterSpec>& specs) {
  filters_.reserve(static_cast<size_t>(specs.size()));
  for (const auto& spec : specs) {
    if (auto filter = createCurveFilter(spec)) {
      filters_.push_back(std::move(filter));
    }
  }
}

bool CurveFilterChain::isEmpty() const {
  return filters_.empty();
}

std::optional<QPointF> CurveFilterChain::process(const QPointF& point) {
  std::optional<QPointF> current = point;
  for (auto& filter : filters_) {
    current = filter->process(*current);
    if (!current.has_value()) {
      return std::nullopt;
    }
  }
  return current;
}

void CurveFilterChain::reset() {
  for (auto& filter : filters_) {
    filter->reset();
  }
}

}  // namespace rqt_multiplot
