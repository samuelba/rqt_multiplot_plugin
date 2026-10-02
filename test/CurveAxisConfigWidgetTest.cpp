#include <QApplication>
#include <QMetaObject>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveAxisConfigWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveAxisConfigWidget;

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

bool invoke(QObject* object, const char* method, int value) {
  return QMetaObject::invokeMethod(object, method, Q_ARG(int, value));
}

}  // namespace

TEST(CurveAxisConfigWidget, fieldModeSlotsWriteTheAxisConfig) {
  ensureApplication();
  CurveAxisConfig config;
  CurveAxisConfigWidget widget;
  widget.setConfig(&config);

  ASSERT_TRUE(invoke(&widget, "checkBoxFieldReceiptTimeStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::MessageReceiptTime);
  ASSERT_TRUE(invoke(&widget, "checkBoxFieldArrayIndexStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::ArrayIndex);
  ASSERT_TRUE(invoke(&widget, "checkBoxFieldDiagnosticValueStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::DiagnosticValue);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "comboBoxDiagnosticStatusEdited", Q_ARG(QString, QStringLiteral("Battery"))));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "comboBoxDiagnosticKeyEdited", Q_ARG(QString, QStringLiteral("Voltage"))));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "comboBoxDiagnosticHardwareIdEdited", Q_ARG(QString, QStringLiteral("host"))));
  EXPECT_EQ(config.getDiagnosticStatus(), QStringLiteral("Battery"));
  EXPECT_EQ(config.getDiagnosticKey(), QStringLiteral("Voltage"));
  EXPECT_EQ(config.getDiagnosticHardwareId(), QStringLiteral("host"));

  ASSERT_TRUE(invoke(&widget, "checkBoxFieldTopicMetricStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_EQ(config.getFieldType(), CurveAxisConfig::TopicMetric);
  ASSERT_TRUE(invoke(&widget, "comboBoxTopicMetricActivated", 1));
  ASSERT_TRUE(invoke(&widget, "spinBoxTopicMetricWindowValueChanged", 20));

  ASSERT_TRUE(invoke(&widget, "checkBoxRadiansToDegreesStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_EQ(config.getUnitConversion(), CurveAxisConfig::RadiansToDegrees);
  ASSERT_TRUE(invoke(&widget, "checkBoxDegreesToRadiansStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_EQ(config.getUnitConversion(), CurveAxisConfig::DegreesToRadians);

  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "comboBoxTopicCurrentTopicChanged", Q_ARG(QString, QStringLiteral("/joint_states"))));
  EXPECT_EQ(config.getTopic(), QStringLiteral("/joint_states"));
  ASSERT_TRUE(
      QMetaObject::invokeMethod(&widget, "comboBoxTypeCurrentTypeChanged", Q_ARG(QString, QStringLiteral("sensor_msgs/msg/JointState"))));
  EXPECT_EQ(config.getType(), QStringLiteral("sensor_msgs/msg/JointState"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "widgetFieldCurrentFieldChanged", Q_ARG(QString, QStringLiteral("position/0"))));
  EXPECT_EQ(config.getField(), QStringLiteral("position/0"));

  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "comboBoxTopicUpdateStarted"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "comboBoxTopicUpdateFinished"));
  QMetaObject::invokeMethod(&widget, "comboBoxTypeUpdateStarted");
  QMetaObject::invokeMethod(&widget, "comboBoxTypeUpdateFinished");
  QMetaObject::invokeMethod(&widget, "widgetFieldLoadingStarted");
  QMetaObject::invokeMethod(&widget, "widgetFieldLoadingFinished");
  QMetaObject::invokeMethod(&widget, "widgetFieldLoadingFailed", Q_ARG(QString, QStringLiteral("missing")));
  QMetaObject::invokeMethod(&widget, "widgetFieldConnecting", Q_ARG(QString, QStringLiteral("/joint_states")));
  QMetaObject::invokeMethod(&widget, "widgetFieldConnected", Q_ARG(QString, QStringLiteral("/joint_states")));
  QMetaObject::invokeMethod(&widget, "widgetFieldConnectionTimeout", Q_ARG(QString, QStringLiteral("/joint_states")), Q_ARG(double, 1.0));

  CurveAxisConfig replacement;
  widget.setConfig(&replacement);
  widget.setConfig(nullptr);
}
