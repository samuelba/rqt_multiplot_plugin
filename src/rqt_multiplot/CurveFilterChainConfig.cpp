/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CurveFilterChainConfig.hpp"

#include <algorithm>

#include <QStringList>

namespace rqt_multiplot {

namespace {

const QString kFilterGroupPrefix = QStringLiteral("filter_");

int filterGroupIndex(const QString& group) {
  if (!group.startsWith(kFilterGroupPrefix)) {
    return -1;
  }
  bool ok = false;
  const int index = group.mid(kFilterGroupPrefix.size()).toInt(&ok);
  return ok ? index : -1;
}

}  // namespace

void saveCurveFilterSpec(QSettings& settings, const CurveFilterSpec& spec) {
  settings.setValue("type", curveFilterTypeKey(spec.type));
  switch (spec.type) {
    case CurveFilterType::Derivative:
    case CurveFilterType::Integral:
      settings.setValue("fixed_step_enabled", spec.useFixedStep);
      settings.setValue("fixed_step", spec.fixedStep);
      break;
    case CurveFilterType::MovingAverage:
      settings.setValue("window_size", spec.windowSize);
      settings.setValue("compensate_lag", spec.compensateLag);
      break;
    case CurveFilterType::MovingRms:
      settings.setValue("window_size", spec.windowSize);
      break;
    case CurveFilterType::MovingVariance:
      settings.setValue("window_size", spec.windowSize);
      settings.setValue("standard_deviation", spec.standardDeviation);
      break;
    case CurveFilterType::LowPass:
      settings.setValue("time_constant", spec.timeConstant);
      break;
    case CurveFilterType::ScaleOffset:
      settings.setValue("scale", spec.scale);
      settings.setValue("offset", spec.offset);
      break;
    case CurveFilterType::Threshold:
      settings.setValue("comparison", static_cast<int>(spec.comparison));
      settings.setValue("threshold_a", spec.thresholdA);
      settings.setValue("threshold_b", spec.thresholdB);
      break;
    case CurveFilterType::OutlierRemoval:
      settings.setValue("outlier_factor", spec.outlierFactor);
      break;
    case CurveFilterType::SamplesCount:
      settings.setValue("window_ms", spec.windowMilliseconds);
      break;
    case CurveFilterType::Absolute:
    case CurveFilterType::TimeSincePrevious:
      break;
  }
}

std::optional<CurveFilterSpec> loadCurveFilterSpec(QSettings& settings) {
  const std::optional<CurveFilterType> type = curveFilterTypeFromKey(settings.value("type").toString());
  if (!type.has_value()) {
    return std::nullopt;
  }

  const CurveFilterSpec defaults = defaultCurveFilterSpec(*type);
  CurveFilterSpec spec = defaults;
  spec.useFixedStep = settings.value("fixed_step_enabled", defaults.useFixedStep).toBool();
  spec.fixedStep = settings.value("fixed_step", defaults.fixedStep).toDouble();
  spec.windowSize = std::max(settings.value("window_size", defaults.windowSize).toInt(), 1);
  spec.compensateLag = settings.value("compensate_lag", defaults.compensateLag).toBool();
  spec.standardDeviation = settings.value("standard_deviation", defaults.standardDeviation).toBool();
  spec.timeConstant = settings.value("time_constant", defaults.timeConstant).toDouble();
  spec.scale = settings.value("scale", defaults.scale).toDouble();
  spec.offset = settings.value("offset", defaults.offset).toDouble();
  const int comparison = settings.value("comparison", static_cast<int>(defaults.comparison)).toInt();
  if (comparison >= static_cast<int>(CurveFilterComparison::Equal) && comparison <= static_cast<int>(CurveFilterComparison::Range)) {
    spec.comparison = static_cast<CurveFilterComparison>(comparison);
  }
  spec.thresholdA = settings.value("threshold_a", defaults.thresholdA).toDouble();
  spec.thresholdB = settings.value("threshold_b", defaults.thresholdB).toDouble();
  spec.outlierFactor = settings.value("outlier_factor", defaults.outlierFactor).toDouble();
  spec.windowMilliseconds = std::max(settings.value("window_ms", defaults.windowMilliseconds).toInt(), 1);
  return spec;
}

void writeCurveFilterSpec(QDataStream& stream, const CurveFilterSpec& spec) {
  stream << curveFilterTypeKey(spec.type) << spec.useFixedStep << spec.fixedStep << static_cast<qint32>(spec.windowSize)
         << spec.compensateLag << spec.standardDeviation << spec.timeConstant << spec.scale << spec.offset
         << static_cast<qint32>(spec.comparison) << spec.thresholdA << spec.thresholdB << spec.outlierFactor
         << static_cast<qint32>(spec.windowMilliseconds);
}

CurveFilterSpec readCurveFilterSpec(QDataStream& stream) {
  QString typeKey;
  qint32 windowSize = 0;
  qint32 comparison = 0;
  qint32 windowMilliseconds = 0;
  CurveFilterSpec spec;

  stream >> typeKey >> spec.useFixedStep >> spec.fixedStep >> windowSize >> spec.compensateLag >> spec.standardDeviation >>
      spec.timeConstant >> spec.scale >> spec.offset >> comparison >> spec.thresholdA >> spec.thresholdB >> spec.outlierFactor >>
      windowMilliseconds;

  spec.type = curveFilterTypeFromKey(typeKey).value_or(CurveFilterType::Derivative);
  spec.windowSize = std::max(static_cast<int>(windowSize), 1);
  spec.comparison = static_cast<CurveFilterComparison>(std::clamp(
      static_cast<int>(comparison), static_cast<int>(CurveFilterComparison::Equal), static_cast<int>(CurveFilterComparison::Range)));
  spec.windowMilliseconds = std::max(static_cast<int>(windowMilliseconds), 1);
  return spec;
}

CurveFilterChainConfig::CurveFilterChainConfig(QObject* parent) : Config(parent) {}

CurveFilterChainConfig::~CurveFilterChainConfig() = default;

const QVector<CurveFilterSpec>& CurveFilterChainConfig::getFilters() const {
  return filters_;
}

void CurveFilterChainConfig::setFilters(const QVector<CurveFilterSpec>& filters) {
  if (filters != filters_) {
    filters_ = filters;

    emit filtersChanged();
    emit changed();
  }
}

int CurveFilterChainConfig::getNumFilters() const {
  return static_cast<int>(filters_.size());
}

bool CurveFilterChainConfig::isEmpty() const {
  return filters_.isEmpty();
}

CurveFilterSpec CurveFilterChainConfig::getFilter(int index) const {
  return filters_.value(index);
}

void CurveFilterChainConfig::setFilter(int index, const CurveFilterSpec& filter) {
  if (index < 0 || index >= filters_.size()) {
    return;
  }
  QVector<CurveFilterSpec> filters = filters_;
  filters[index] = filter;
  setFilters(filters);
}

void CurveFilterChainConfig::addFilter(const CurveFilterSpec& filter) {
  QVector<CurveFilterSpec> filters = filters_;
  filters.append(filter);
  setFilters(filters);
}

void CurveFilterChainConfig::removeFilter(int index) {
  if (index < 0 || index >= filters_.size()) {
    return;
  }
  QVector<CurveFilterSpec> filters = filters_;
  filters.remove(index);
  setFilters(filters);
}

void CurveFilterChainConfig::moveFilter(int from, int to) {
  if (from < 0 || from >= filters_.size() || to < 0 || to >= filters_.size() || from == to) {
    return;
  }
  QVector<CurveFilterSpec> filters = filters_;
  filters.move(from, to);
  setFilters(filters);
}

void CurveFilterChainConfig::save(QSettings& settings) const {
  for (int index = 0; index < filters_.size(); ++index) {
    settings.beginGroup(kFilterGroupPrefix + QString::number(index));
    saveCurveFilterSpec(settings, filters_[index]);
    settings.endGroup();
  }
}

void CurveFilterChainConfig::load(QSettings& settings) {
  QStringList groups = settings.childGroups();
  std::sort(groups.begin(), groups.end(),
            [](const QString& lhs, const QString& rhs) { return filterGroupIndex(lhs) < filterGroupIndex(rhs); });

  QVector<CurveFilterSpec> filters;
  for (const QString& group : groups) {
    if (filterGroupIndex(group) < 0) {
      continue;
    }
    settings.beginGroup(group);
    if (const auto spec = loadCurveFilterSpec(settings)) {
      filters.append(*spec);
    }
    settings.endGroup();
  }
  setFilters(filters);
}

void CurveFilterChainConfig::reset() {
  setFilters({});
}

void CurveFilterChainConfig::write(QDataStream& stream) const {
  stream << static_cast<qint32>(filters_.size());
  for (const auto& filter : filters_) {
    writeCurveFilterSpec(stream, filter);
  }
}

void CurveFilterChainConfig::read(QDataStream& stream) {
  qint32 count = 0;
  stream >> count;

  QVector<CurveFilterSpec> filters;
  for (qint32 index = 0; index < count && stream.status() == QDataStream::Ok; ++index) {
    filters.append(readCurveFilterSpec(stream));
  }
  setFilters(filters);
}

CurveFilterChainConfig& CurveFilterChainConfig::operator=(const CurveFilterChainConfig& src) {
  setFilters(src.filters_);
  return *this;
}

}  // namespace rqt_multiplot
