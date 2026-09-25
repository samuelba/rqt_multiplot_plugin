#include <memory>

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotWidget;
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
