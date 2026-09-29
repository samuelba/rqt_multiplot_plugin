#include <functional>
#include <memory>

#include <QApplication>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QListWidget>
#include <QMimeData>
#include <QPushButton>
#include <QTimer>

#include <gtest/gtest.h>

#include "rqt_multiplot/ArrayDropDialog.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/Theme.hpp"
#include "rqt_multiplot/TopicDropDialog.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"

namespace {

using rqt_multiplot::ArrayDropDialog;
using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotWidget;
using rqt_multiplot::Theme;
using rqt_multiplot::TopicDropDialog;
using rqt_multiplot::TopicFieldRef;

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

QMimeData* topicFieldsMime(const QVector<TopicFieldRef>& refs) {
  auto* mimeData = new QMimeData();
  mimeData->setData(rqt_multiplot::kTopicFieldsMimeType, rqt_multiplot::encodeTopicFields(refs));
  return mimeData;
}

bool sendDragEnter(PlotWidget& widget, const QMimeData* mimeData) {
  QDragEnterEvent event(widget.rect().center(), Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(&widget, &event);
  return event.isAccepted();
}

bool sendDrop(PlotWidget& widget, const QMimeData* mimeData) {
  if (!sendDragEnter(widget, mimeData)) {
    return false;
  }
  QDropEvent event(QPointF(widget.rect().center()), Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(&widget, &event);
  return event.isAccepted();
}

TEST(PlotWidgetTopicDrop, dragEnterAcceptsTopicFields) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicFieldsMime({{"/imu", "sensor_msgs/msg/Imu", "orientation/x"}}));

  EXPECT_TRUE(sendDragEnter(widget, mimeData.get()));
}

TEST(PlotWidgetTopicDrop, dragEnterRejectsUnknownMime) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  QMimeData mimeData;
  mimeData.setText("/imu");

  EXPECT_FALSE(sendDragEnter(widget, &mimeData));
}

TEST(PlotWidgetTopicDrop, dropAddsOneCurvePerField) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicFieldsMime({{"/pose", "geometry_msgs/msg/Point", "x"},
                                                             {"/pose", "geometry_msgs/msg/Point", "y"},
                                                             {"/joint_states", "sensor_msgs/msg/JointState", "position/*"}}));

  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 3U);
  EXPECT_EQ(config.getCurveConfig(0)->getTitle(), QString("/pose/x"));
  EXPECT_EQ(config.getCurveConfig(0)->getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::MessageReceiptTime);
  EXPECT_EQ(config.getCurveConfig(1)->getAxisConfig(CurveConfig::Y)->getField(), QString("y"));
  EXPECT_EQ(config.getCurveConfig(2)->getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::ArrayIndex);
}

TEST(PlotWidgetTopicDrop, diagnosticKeyDropAddsDiagnosticValueCurve) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(
      topicFieldsMime({{"/diagnostics", "diagnostic_msgs/msg/DiagnosticArray", QString(), {"cpu", "host1", "load"}}}));

  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 1U);
  const CurveAxisConfig* y = config.getCurveConfig(0)->getAxisConfig(CurveConfig::Y);
  EXPECT_EQ(y->getFieldType(), CurveAxisConfig::DiagnosticValue);
  EXPECT_EQ(y->getDiagnosticStatus(), QString("cpu"));
  EXPECT_EQ(y->getDiagnosticKey(), QString("load"));
  EXPECT_EQ(y->getDiagnosticHardwareId(), QString("host1"));
}

QMimeData* arrayMime() {
  QMimeData* mimeData = topicFieldsMime({{"/scan", "sensor_msgs/msg/LaserScan", "ranges/*"}});
  mimeData->setData(rqt_multiplot::kTopicFieldsExpandedMimeType,
                    rqt_multiplot::encodeTopicFields(
                        {{"/scan", "sensor_msgs/msg/LaserScan", "ranges/0"}, {"/scan", "sensor_msgs/msg/LaserScan", "ranges/1"}}));
  return mimeData;
}

void answerArrayDropDialog(const QString& buttonName) {
  QTimer::singleShot(0, [buttonName]() {
    auto* dialog = qobject_cast<ArrayDropDialog*>(QApplication::activeModalWidget());
    ASSERT_NE(dialog, nullptr);
    if (buttonName.isEmpty()) {
      dialog->reject();
      return;
    }
    auto* button = dialog->findChild<QPushButton*>(buttonName);
    ASSERT_NE(button, nullptr);
    button->click();
  });
}

TEST(PlotWidgetTopicDrop, arrayDropDialogFollowsDarkTheme) {
  ensureApplication();
  Theme::apply(nullptr, Theme::Id::Dark);

  const ArrayDropDialog dialog(nullptr, 1, 2);

  EXPECT_EQ(dialog.palette().color(QPalette::Window), Theme::palette(Theme::Id::Dark).color(QPalette::Window));
  Theme::apply(nullptr, Theme::Id::Light);
}

TEST(PlotWidgetTopicDrop, arrayDropDialogArrayIndexKeepsWildcardCurve) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(arrayMime());

  answerArrayDropDialog("arrayDropArrayIndexButton");
  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 1U);
  EXPECT_EQ(config.getCurveConfig(0)->getAxisConfig(CurveConfig::Y)->getField(), QString("ranges/*"));
  EXPECT_EQ(config.getCurveConfig(0)->getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::ArrayIndex);
}

TEST(PlotWidgetTopicDrop, arrayDropDialogIndividualAddsCurvePerElement) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(arrayMime());

  answerArrayDropDialog("arrayDropIndividualButton");
  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 2U);
  EXPECT_EQ(config.getCurveConfig(1)->getAxisConfig(CurveConfig::Y)->getField(), QString("ranges/1"));
  EXPECT_EQ(config.getCurveConfig(1)->getAxisConfig(CurveConfig::X)->getFieldType(), CurveAxisConfig::MessageReceiptTime);
}

TEST(PlotWidgetTopicDrop, arrayDropDialogCancelAddsNothing) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(arrayMime());

  answerArrayDropDialog(QString());
  EXPECT_FALSE(sendDrop(widget, mimeData.get()));

  EXPECT_EQ(config.getNumCurves(), 0U);
}

constexpr int kRootFieldCount = 12;

QMimeData* topicRootMime() {
  QVector<TopicFieldRef> fields;
  for (int i = 0; i < kRootFieldCount; ++i) {
    fields.append({"/pose", "test/msg/Big", QStringLiteral("f%1").arg(i)});
  }
  QMimeData* mimeData = topicFieldsMime(fields);
  mimeData->setData(rqt_multiplot::kTopicRootMimeType,
                    rqt_multiplot::encodeTopicFields(rqt_multiplot::topicMetricRefs("/pose", "test/msg/Big", false)));
  return mimeData;
}

void answerTopicDropDialog(const std::function<void(TopicDropDialog*)>& answer) {
  QTimer::singleShot(0, [answer]() {
    auto* dialog = qobject_cast<TopicDropDialog*>(QApplication::activeModalWidget());
    ASSERT_NE(dialog, nullptr);
    answer(dialog);
  });
}

void clickOk(TopicDropDialog* dialog) {
  auto* buttonBox = dialog->findChild<QDialogButtonBox*>("topicDropButtonBox");
  ASSERT_NE(buttonBox, nullptr);
  buttonBox->button(QDialogButtonBox::Ok)->click();
}

TEST(PlotWidgetTopicDrop, topicRootDropDefaultsToAllFieldsWithoutConfirmation) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicRootMime());

  answerTopicDropDialog([](TopicDropDialog* dialog) {
    auto* metrics = dialog->findChild<QListWidget*>("topicDropMetricsList");
    ASSERT_NE(metrics, nullptr);
    EXPECT_EQ(metrics->item(0)->checkState(), Qt::Unchecked);
    clickOk(dialog);
  });
  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  EXPECT_EQ(config.getNumCurves(), static_cast<size_t>(kRootFieldCount));
}

TEST(PlotWidgetTopicDrop, topicRootDropAddsCheckedMetricsBeforeFields) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicRootMime());

  answerTopicDropDialog([](TopicDropDialog* dialog) {
    auto* fieldsNone = dialog->findChild<QPushButton*>("topicDropFieldsSelectNone");
    auto* fields = dialog->findChild<QListWidget*>("topicDropFieldsList");
    auto* metrics = dialog->findChild<QListWidget*>("topicDropMetricsList");
    ASSERT_NE(fieldsNone, nullptr);
    ASSERT_NE(fields, nullptr);
    ASSERT_NE(metrics, nullptr);
    fieldsNone->click();
    fields->item(3)->setCheckState(Qt::Checked);
    metrics->item(static_cast<int>(rqt_multiplot::TopicMetric::Bandwidth))->setCheckState(Qt::Checked);
    clickOk(dialog);
  });
  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 2U);
  const CurveAxisConfig* metricAxis = config.getCurveConfig(0)->getAxisConfig(CurveConfig::Y);
  EXPECT_EQ(metricAxis->getFieldType(), CurveAxisConfig::TopicMetric);
  EXPECT_EQ(metricAxis->getTopicMetric(), rqt_multiplot::TopicMetric::Bandwidth);
  EXPECT_EQ(config.getCurveConfig(0)->getTitle(), QString("/pose/bandwidth"));
  EXPECT_EQ(config.getCurveConfig(1)->getAxisConfig(CurveConfig::Y)->getField(), QString("f3"));
}

TEST(PlotWidgetTopicDrop, topicRootDropLeavesArrayFieldsUnchecked) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicFieldsMime(
      {{"/joints", "sensor_msgs/msg/JointState", "header/stamp"}, {"/joints", "sensor_msgs/msg/JointState", "position/*"}}));
  mimeData->setData(rqt_multiplot::kTopicRootMimeType, rqt_multiplot::encodeTopicFields({}));

  answerTopicDropDialog([](TopicDropDialog* dialog) {
    auto* fields = dialog->findChild<QListWidget*>("topicDropFieldsList");
    ASSERT_NE(fields, nullptr);
    ASSERT_EQ(fields->count(), 2);
    EXPECT_EQ(fields->item(0)->checkState(), Qt::Checked);
    EXPECT_EQ(fields->item(1)->checkState(), Qt::Unchecked);
    clickOk(dialog);
  });
  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 1U);
  EXPECT_EQ(config.getCurveConfig(0)->getAxisConfig(CurveConfig::Y)->getField(), QString("header/stamp"));
}

TEST(PlotWidgetTopicDrop, topicRootDropOkIsDisabledWithNothingChecked) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicRootMime());

  answerTopicDropDialog([](TopicDropDialog* dialog) {
    auto* fieldsNone = dialog->findChild<QPushButton*>("topicDropFieldsSelectNone");
    auto* buttonBox = dialog->findChild<QDialogButtonBox*>("topicDropButtonBox");
    ASSERT_NE(fieldsNone, nullptr);
    ASSERT_NE(buttonBox, nullptr);
    fieldsNone->click();
    EXPECT_FALSE(buttonBox->button(QDialogButtonBox::Ok)->isEnabled());
    dialog->reject();
  });
  EXPECT_FALSE(sendDrop(widget, mimeData.get()));

  EXPECT_EQ(config.getNumCurves(), 0U);
}

TEST(PlotWidgetTopicDrop, metricLeafDropAddsCurveWithoutDialog) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(
      topicFieldsMime({TopicFieldRef::forMetric("/pose", "test/msg/Big", rqt_multiplot::TopicMetric::Rate)}));

  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  ASSERT_EQ(config.getNumCurves(), 1U);
  EXPECT_EQ(config.getCurveConfig(0)->getAxisConfig(CurveConfig::Y)->getFieldType(), CurveAxisConfig::TopicMetric);
}

TEST(PlotWidgetTopicDrop, droppingSameFieldTwiceKeepsTitlesUnique) {
  ensureApplication();
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(topicFieldsMime({{"/pose", "geometry_msgs/msg/Point", "x"}}));

  sendDrop(widget, mimeData.get());
  sendDrop(widget, mimeData.get());

  ASSERT_EQ(config.getNumCurves(), 2U);
  EXPECT_EQ(config.getCurveConfig(1)->getTitle(), QString("Copy of /pose/x"));
}

}  // namespace
