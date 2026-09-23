#include <cstdlib>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QObject>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveConfigWidget.hpp"
#include "rqt_multiplot/MessageFieldWidget.hpp"
#include "rqt_multiplot/MessageTypeComboBox.hpp"
#include "rqt_multiplot/StatusWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveConfigWidget;
using rqt_multiplot::MessageFieldWidget;
using rqt_multiplot::MessageTypeComboBox;
using rqt_multiplot::StatusWidget;

constexpr auto kUnpairedArrayMessage = "Array index or * field must be paired with another * field or array index";

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

class FieldDefinitionLoadWaiter : public QObject {
 public:
  explicit FieldDefinitionLoadWaiter(CurveConfigWidget& widget) {
    const auto fieldWidgets = widget.findChildren<MessageFieldWidget*>();
    subscribed_ = !fieldWidgets.isEmpty();
    pending_ = fieldWidgets.size();
    for (auto* fieldWidget : fieldWidgets) {
      connect(fieldWidget, &MessageFieldWidget::loadingFinished, this, [this]() { --pending_; });
      connect(fieldWidget, &MessageFieldWidget::loadingFailed, this, [this]() { --pending_; });
    }
  }

  bool wait(int timeoutMs = 5000) {
    if (!subscribed_) {
      return false;
    }

    QElapsedTimer timer;
    timer.start();
    while (pending_ > 0 && timer.elapsed() < timeoutMs) {
      QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
    return pending_ == 0;
  }

 private:
  bool subscribed_ = false;
  int pending_ = 0;
};

void configureUnpairedArrayIndex(CurveConfig& config) {
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setType("sensor_msgs/msg/JointState");
  config.getAxisConfig(CurveConfig::Y)->setType("sensor_msgs/msg/JointState");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config.getAxisConfig(CurveConfig::Y)->setField("position/0");
}

TEST(CurveConfigWidget, setConfigMarksUnpairedArrayIndexAsFieldError) {
  ensureApplication();

  CurveConfig config;
  configureUnpairedArrayIndex(config);

  CurveConfigWidget widget;
  widget.setConfig(config);

  EXPECT_EQ(widget.getAxisConfigWidget(CurveConfig::X)->getFieldStatusRole(), StatusWidget::Error);
  EXPECT_EQ(widget.getAxisConfigWidget(CurveConfig::X)->getFieldStatusMessage(), QString(kUnpairedArrayMessage));
  EXPECT_TRUE(widget.isValidationErrorVisible());
  EXPECT_TRUE(widget.validationErrorText().contains(QString(kUnpairedArrayMessage)));
}

TEST(CurveConfigWidget, unpairedArrayIndexStaysAnErrorAfterFieldLoad) {
  ensureApplication();

  CurveConfig config;
  configureUnpairedArrayIndex(config);

  CurveConfigWidget widget;
  FieldDefinitionLoadWaiter fieldLoadWaiter(widget);
  widget.setConfig(config);
  ASSERT_TRUE(fieldLoadWaiter.wait());

  EXPECT_EQ(widget.getAxisConfigWidget(CurveConfig::X)->getFieldStatusRole(), StatusWidget::Error);
  EXPECT_EQ(widget.getAxisConfigWidget(CurveConfig::X)->getFieldStatusMessage(), QString(kUnpairedArrayMessage));
  EXPECT_TRUE(widget.validationErrorText().contains(QString(kUnpairedArrayMessage)));
}

TEST(CurveConfigWidget, pairingErrorClearsWhenAxesBecomeCompatible) {
  ensureApplication();

  CurveConfig config;
  configureUnpairedArrayIndex(config);

  CurveConfigWidget widget;
  widget.setConfig(config);
  widget.getConfig().getAxisConfig(CurveConfig::Y)->setField("position/*");

  EXPECT_EQ(widget.getAxisConfigWidget(CurveConfig::X)->getFieldStatusRole(), StatusWidget::Okay);
  EXPECT_FALSE(widget.validationErrorText().contains(QString(kUnpairedArrayMessage)));
}

TEST(CurveConfigWidget, emptyConfigDoesNotShowArrayPairingError) {
  ensureApplication();

  CurveConfigWidget widget;

  EXPECT_FALSE(widget.validationErrorText().contains(QString(kUnpairedArrayMessage)));
}

TEST(CurveConfigWidget, timeSeriesDoesNotShowArrayPairingError) {
  ensureApplication();

  CurveConfig config;
  config.getAxisConfig(CurveConfig::X)->setTopic("/array");
  config.getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config.getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::MessageReceiptTime);
  config.getAxisConfig(CurveConfig::Y)->setField("position/0");

  CurveConfigWidget widget;
  widget.setConfig(config);

  EXPECT_FALSE(widget.validationErrorText().contains(QString(kUnpairedArrayMessage)));
}

TEST(CurveConfigWidget, curveAxisHasNoStartTimeFromZeroCheckbox) {
  ensureApplication();

  CurveConfigWidget widget;

  EXPECT_TRUE(widget.findChildren<QCheckBox*>("checkBoxLabelFromZero").isEmpty());
}

TEST(CurveConfigWidget, axisWidgetsExposeUnitConversionCheckboxes) {
  ensureApplication();

  CurveConfigWidget widget;

  for (const auto axis : {CurveConfig::X, CurveConfig::Y}) {
    const auto* axisWidget = widget.getAxisConfigWidget(axis);
    ASSERT_NE(axisWidget, nullptr);
    EXPECT_NE(axisWidget->findChild<QCheckBox*>("checkBoxRadiansToDegrees"), nullptr);
    EXPECT_NE(axisWidget->findChild<QCheckBox*>("checkBoxDegreesToRadians"), nullptr);
  }
}

TEST(CurveConfigWidget, unitConversionCheckboxesAreExclusive) {
  ensureApplication();

  CurveConfigWidget widget;

  auto* radToDeg = widget.getAxisConfigWidget(CurveConfig::Y)->findChild<QCheckBox*>("checkBoxRadiansToDegrees");
  auto* degToRad = widget.getAxisConfigWidget(CurveConfig::Y)->findChild<QCheckBox*>("checkBoxDegreesToRadians");
  ASSERT_NE(radToDeg, nullptr);
  ASSERT_NE(degToRad, nullptr);

  radToDeg->setCheckState(Qt::Checked);
  EXPECT_EQ(radToDeg->checkState(), Qt::Checked);
  EXPECT_EQ(degToRad->checkState(), Qt::Unchecked);
  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getUnitConversion(), CurveAxisConfig::RadiansToDegrees);

  degToRad->setCheckState(Qt::Checked);
  EXPECT_EQ(degToRad->checkState(), Qt::Checked);
  EXPECT_EQ(radToDeg->checkState(), Qt::Unchecked);
  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getUnitConversion(), CurveAxisConfig::DegreesToRadians);
}

TEST(CurveConfigWidget, unitConversionCheckboxesDisabledForSyntheticFields) {
  ensureApplication();

  CurveConfig config;
  configureUnpairedArrayIndex(config);

  CurveConfigWidget widget;
  widget.setConfig(config);

  const auto* xWidget = widget.getAxisConfigWidget(CurveConfig::X);
  auto* radToDeg = xWidget->findChild<QCheckBox*>("checkBoxRadiansToDegrees");
  auto* degToRad = xWidget->findChild<QCheckBox*>("checkBoxDegreesToRadians");
  ASSERT_NE(radToDeg, nullptr);
  ASSERT_NE(degToRad, nullptr);

  EXPECT_FALSE(radToDeg->isEnabled());
  EXPECT_FALSE(degToRad->isEnabled());
}

TEST(CurveConfigWidget, diagnosticCheckboxHiddenForOtherTypes) {
  ensureApplication();

  CurveConfig config;
  config.getAxisConfig(CurveConfig::Y)->setType("sensor_msgs/msg/JointState");

  CurveConfigWidget widget;
  widget.setConfig(config);

  auto* diagnostic = widget.getAxisConfigWidget(CurveConfig::Y)->findChild<QCheckBox*>("checkBoxFieldDiagnosticValue");
  ASSERT_NE(diagnostic, nullptr);
  EXPECT_TRUE(diagnostic->isHidden());
}

TEST(CurveConfigWidget, diagnosticValueUnchecksReceiptTimeAndAcceptsTypedNames) {
  ensureApplication();

  CurveConfig config;
  config.getAxisConfig(CurveConfig::Y)->setType("diagnostic_msgs/msg/DiagnosticArray");
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::MessageReceiptTime);

  CurveConfigWidget widget;
  widget.setConfig(config);

  auto* axisWidget = widget.getAxisConfigWidget(CurveConfig::Y);
  auto* diagnostic = axisWidget->findChild<QCheckBox*>("checkBoxFieldDiagnosticValue");
  auto* receipt = axisWidget->findChild<QCheckBox*>("checkBoxFieldReceiptTime");
  auto* status = axisWidget->findChild<QComboBox*>("comboBoxDiagnosticStatus");
  auto* key = axisWidget->findChild<QComboBox*>("comboBoxDiagnosticKey");
  auto* radToDeg = axisWidget->findChild<QCheckBox*>("checkBoxRadiansToDegrees");
  ASSERT_NE(diagnostic, nullptr);
  ASSERT_NE(receipt, nullptr);
  ASSERT_NE(status, nullptr);
  ASSERT_NE(key, nullptr);
  ASSERT_NE(radToDeg, nullptr);
  EXPECT_FALSE(diagnostic->isHidden());

  diagnostic->setCheckState(Qt::Checked);
  EXPECT_EQ(receipt->checkState(), Qt::Unchecked);
  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getFieldType(), CurveAxisConfig::DiagnosticValue);
  EXPECT_TRUE(radToDeg->isEnabled());
  EXPECT_EQ(axisWidget->getFieldStatusRole(), StatusWidget::Error);

  status->setCurrentText("/Power System/Battery");
  key->setCurrentText("Voltage");
  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getDiagnosticStatus(), QString("/Power System/Battery"));
  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getDiagnosticKey(), QString("Voltage"));
  EXPECT_TRUE(widget.getConfig().getAxisConfig(CurveConfig::Y)->getDiagnosticHardwareId().isEmpty());
  EXPECT_EQ(axisWidget->getFieldStatusRole(), StatusWidget::Okay);

  auto* hardwareId = axisWidget->findChild<QComboBox*>("comboBoxDiagnosticHardwareId");
  ASSERT_NE(hardwareId, nullptr);
  hardwareId->setCurrentText("pack-a");
  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getDiagnosticHardwareId(), QString("pack-a"));
  EXPECT_EQ(axisWidget->getFieldStatusRole(), StatusWidget::Okay);
}

TEST(CurveConfigWidget, changingTypeAwayFromDiagnosticArrayResetsField) {
  ensureApplication();

  CurveConfig config;
  config.getAxisConfig(CurveConfig::Y)->setType("diagnostic_msgs/msg/DiagnosticArray");
  config.getAxisConfig(CurveConfig::Y)->setFieldType(CurveAxisConfig::DiagnosticValue);
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticStatus("Battery");
  config.getAxisConfig(CurveConfig::Y)->setDiagnosticKey("Voltage");

  CurveConfigWidget widget;
  widget.setConfig(config);

  auto* axisWidget = widget.getAxisConfigWidget(CurveConfig::Y);
  auto* typeCombo = axisWidget->findChild<MessageTypeComboBox*>("comboBoxType");
  ASSERT_NE(typeCombo, nullptr);
  typeCombo->setCurrentType("std_msgs/msg/Header");

  EXPECT_EQ(widget.getConfig().getAxisConfig(CurveConfig::Y)->getFieldType(), CurveAxisConfig::MessageData);
  EXPECT_TRUE(axisWidget->findChild<QCheckBox*>("checkBoxFieldDiagnosticValue")->isHidden());
  EXPECT_EQ(axisWidget->getFieldStatusRole(), StatusWidget::Error);
}

}  // namespace
