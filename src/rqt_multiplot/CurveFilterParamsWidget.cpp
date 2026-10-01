/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CurveFilterParamsWidget.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>

namespace rqt_multiplot {

namespace {

constexpr int kMaxWindowSize = 1000000;
constexpr int kMaxWindowMilliseconds = 100000000;
constexpr double kMaxMagnitude = 1e12;
constexpr int kDecimals = 6;

}  // namespace

CurveFilterParamsWidget::CurveFilterParamsWidget(QWidget* parent) : QWidget(parent), layout_(new QFormLayout(this)) {
  setObjectName(QStringLiteral("curveFilterParamsWidget"));
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
  rebuildForm();
}

CurveFilterParamsWidget::~CurveFilterParamsWidget() = default;

void CurveFilterParamsWidget::setSpec(const CurveFilterSpec& spec) {
  if (spec == spec_) {
    return;
  }
  spec_ = spec;
  rebuildForm();
}

const CurveFilterSpec& CurveFilterParamsWidget::getSpec() const {
  return spec_;
}

void CurveFilterParamsWidget::rebuildForm() {
  clearForm();

  auto* description = new QLabel(curveFilterTypeDescription(spec_.type), this);
  description->setObjectName(QStringLiteral("curveFilterDescriptionLabel"));
  description->setWordWrap(true);
  layout_->addRow(description);

  switch (spec_.type) {
    case CurveFilterType::Derivative:
    case CurveFilterType::Integral:
      addFixedStepRows();
      break;
    case CurveFilterType::MovingAverage:
    case CurveFilterType::MovingRms:
      addWindowRows(false);
      break;
    case CurveFilterType::MovingVariance:
      addWindowRows(true);
      break;
    case CurveFilterType::LowPass:
      addDoubleRow(tr("Time constant [s]"), QStringLiteral("timeConstantSpinBox"), spec_.timeConstant, 0.0, kMaxMagnitude,
                   [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.timeConstant = value; }); });
      break;
    case CurveFilterType::ScaleOffset:
      addDoubleRow(tr("Scale"), QStringLiteral("scaleSpinBox"), spec_.scale, -kMaxMagnitude, kMaxMagnitude,
                   [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.scale = value; }); });
      addDoubleRow(tr("Offset"), QStringLiteral("offsetSpinBox"), spec_.offset, -kMaxMagnitude, kMaxMagnitude,
                   [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.offset = value; }); });
      break;
    case CurveFilterType::Threshold:
      addThresholdRows();
      break;
    case CurveFilterType::OutlierRemoval:
      addDoubleRow(tr("Spike factor"), QStringLiteral("outlierFactorSpinBox"), spec_.outlierFactor, 0.0, kMaxMagnitude,
                   [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.outlierFactor = value; }); });
      break;
    case CurveFilterType::SamplesCount:
      addIntRow(tr("Window [ms]"), QStringLiteral("windowMillisecondsSpinBox"), spec_.windowMilliseconds, 1, kMaxWindowMilliseconds,
                [this](int value) { applyEdit([value](CurveFilterSpec& spec) { spec.windowMilliseconds = value; }); });
      break;
    case CurveFilterType::Absolute:
    case CurveFilterType::TimeSincePrevious: {
      auto* label = new QLabel(tr("No parameters"), this);
      label->setObjectName(QStringLiteral("noParametersLabel"));
      label->setEnabled(false);
      layout_->addRow(label);
      break;
    }
  }
}

void CurveFilterParamsWidget::clearForm() {
  while (layout_->rowCount() > 0) {
    layout_->removeRow(0);
  }
}

void CurveFilterParamsWidget::addFixedStepRows() {
  QCheckBox* fixedCheckBox =
      addCheckBoxRow(tr("Fixed time step"), QStringLiteral("fixedStepCheckBox"), spec_.useFixedStep,
                     [this](bool value) { applyEdit([value](CurveFilterSpec& spec) { spec.useFixedStep = value; }); });
  QDoubleSpinBox* stepSpinBox =
      addDoubleRow(tr("Time step [s]"), QStringLiteral("fixedStepSpinBox"), spec_.fixedStep, 1e-9, kMaxMagnitude,
                   [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.fixedStep = value; }); });
  stepSpinBox->setEnabled(spec_.useFixedStep);
  connect(fixedCheckBox, &QCheckBox::toggled, stepSpinBox, &QWidget::setEnabled);
}

void CurveFilterParamsWidget::addWindowRows(bool hasStandardDeviation) {
  addIntRow(tr("Window size"), QStringLiteral("windowSizeSpinBox"), spec_.windowSize, 1, kMaxWindowSize,
            [this](int value) { applyEdit([value](CurveFilterSpec& spec) { spec.windowSize = value; }); });
  addCheckBoxRow(tr("Compensate lag"), QStringLiteral("compensateLagCheckBox"), spec_.compensateLag,
                 [this](bool value) { applyEdit([value](CurveFilterSpec& spec) { spec.compensateLag = value; }); });
  if (hasStandardDeviation) {
    addCheckBoxRow(tr("Standard deviation"), QStringLiteral("standardDeviationCheckBox"), spec_.standardDeviation,
                   [this](bool value) { applyEdit([value](CurveFilterSpec& spec) { spec.standardDeviation = value; }); });
  }
}

void CurveFilterParamsWidget::addThresholdRows() {
  auto* comparisonComboBox = new QComboBox(this);
  comparisonComboBox->setObjectName(QStringLiteral("comparisonComboBox"));
  comparisonComboBox->addItem(QStringLiteral("y = A"), static_cast<int>(CurveFilterComparison::Equal));
  comparisonComboBox->addItem(QStringLiteral("y < A"), static_cast<int>(CurveFilterComparison::Less));
  comparisonComboBox->addItem(QStringLiteral("y \u2264 A"), static_cast<int>(CurveFilterComparison::LessEqual));
  comparisonComboBox->addItem(QStringLiteral("y > A"), static_cast<int>(CurveFilterComparison::Greater));
  comparisonComboBox->addItem(QStringLiteral("y \u2265 A"), static_cast<int>(CurveFilterComparison::GreaterEqual));
  comparisonComboBox->addItem(tr("A \u2264 y \u2264 B"), static_cast<int>(CurveFilterComparison::Range));
  comparisonComboBox->setCurrentIndex(comparisonComboBox->findData(static_cast<int>(spec_.comparison)));
  layout_->addRow(tr("Condition"), comparisonComboBox);

  addDoubleRow(tr("Value A"), QStringLiteral("thresholdASpinBox"), spec_.thresholdA, -kMaxMagnitude, kMaxMagnitude,
               [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.thresholdA = value; }); });
  QDoubleSpinBox* thresholdBSpinBox =
      addDoubleRow(tr("Value B"), QStringLiteral("thresholdBSpinBox"), spec_.thresholdB, -kMaxMagnitude, kMaxMagnitude,
                   [this](double value) { applyEdit([value](CurveFilterSpec& spec) { spec.thresholdB = value; }); });
  thresholdBSpinBox->setEnabled(spec_.comparison == CurveFilterComparison::Range);

  connect(comparisonComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [this, comparisonComboBox, thresholdBSpinBox](int index) {
            const auto comparison = static_cast<CurveFilterComparison>(comparisonComboBox->itemData(index).toInt());
            thresholdBSpinBox->setEnabled(comparison == CurveFilterComparison::Range);
            applyEdit([comparison](CurveFilterSpec& spec) { spec.comparison = comparison; });
          });
}

QCheckBox* CurveFilterParamsWidget::addCheckBoxRow(const QString& label, const QString& objectName, bool value,
                                                   const std::function<void(bool)>& apply) {
  auto* checkBox = new QCheckBox(label, this);
  checkBox->setObjectName(objectName);
  checkBox->setChecked(value);
  connect(checkBox, &QCheckBox::toggled, this, apply);
  layout_->addRow(checkBox);
  return checkBox;
}

QSpinBox* CurveFilterParamsWidget::addIntRow(const QString& label, const QString& objectName, int value, int minimum, int maximum,
                                             const std::function<void(int)>& apply) {
  auto* spinBox = new QSpinBox(this);
  spinBox->setObjectName(objectName);
  spinBox->setRange(minimum, maximum);
  spinBox->setValue(value);
  connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, apply);
  layout_->addRow(label, spinBox);
  return spinBox;
}

QDoubleSpinBox* CurveFilterParamsWidget::addDoubleRow(const QString& label, const QString& objectName, double value, double minimum,
                                                      double maximum, const std::function<void(double)>& apply) {
  auto* spinBox = new QDoubleSpinBox(this);
  spinBox->setObjectName(objectName);
  spinBox->setDecimals(kDecimals);
  spinBox->setRange(minimum, maximum);
  spinBox->setValue(value);
  connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, apply);
  layout_->addRow(label, spinBox);
  return spinBox;
}

void CurveFilterParamsWidget::applyEdit(const std::function<void(CurveFilterSpec&)>& edit) {
  CurveFilterSpec spec = spec_;
  edit(spec);
  if (spec == spec_) {
    return;
  }
  spec_ = spec;
  emit specChanged();
}

}  // namespace rqt_multiplot
