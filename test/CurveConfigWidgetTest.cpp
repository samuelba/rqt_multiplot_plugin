#include <cstdlib>

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QObject>

#include <gtest/gtest.h>

#include <rqt_multiplot/CurveAxisConfig.h>
#include <rqt_multiplot/CurveConfig.h>
#include <rqt_multiplot/CurveConfigWidget.h>
#include <rqt_multiplot/MessageFieldWidget.h>
#include <rqt_multiplot/StatusWidget.h>

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveConfigWidget;
using rqt_multiplot::MessageFieldWidget;
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

}  // namespace
