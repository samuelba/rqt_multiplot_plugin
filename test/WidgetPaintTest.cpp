#include <QApplication>
#include <QImage>
#include <QKeyEvent>
#include <QMetaObject>
#include <QPainter>
#include <QStandardItemModel>
#include <QStyleOptionViewItem>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisScaleConfig.hpp"
#include "rqt_multiplot/CurveAxisScaleConfigWidget.hpp"
#include "rqt_multiplot/MatchFilterComboBox.hpp"
#include "rqt_multiplot/PenStyleComboBox.hpp"
#include "rqt_multiplot/PenStyleItemDelegate.hpp"
#include "rqt_multiplot/StatusWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisScaleConfig;
using rqt_multiplot::CurveAxisScaleConfigWidget;
using rqt_multiplot::MatchFilterComboBox;
using rqt_multiplot::PenStyleComboBox;
using rqt_multiplot::PenStyleItemDelegate;

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

}  // namespace

TEST(PenStyleItemDelegate, paintsSelectedAndPlainStyles) {
  ensureApplication();
  QStandardItemModel model(1, 1);
  model.setData(model.index(0, 0), static_cast<int>(Qt::DashLine), Qt::UserRole);
  ASSERT_TRUE(model.index(0, 0).isValid());

  PenStyleItemDelegate delegate;
  QImage image(100, 40, QImage::Format_ARGB32);
  image.fill(Qt::white);
  QPainter painter(&image);
  ASSERT_TRUE(painter.isActive());
  QStyleOptionViewItem option;
  option.rect = QRect(0, 0, 100, 20);
  option.state = QStyle::State_Enabled;
  delegate.paint(&painter, option, model.index(0, 0));
  option.state = QStyle::State_Selected | QStyle::State_Enabled;
  delegate.paint(&painter, option, model.index(0, 0));
  painter.end();
}

TEST(MatchFilterComboBox, editableFilterAcceptsAKey) {
  ensureApplication();
  MatchFilterComboBox box;
  box.addItem(QStringLiteral("alpha"));
  box.addItem(QStringLiteral("beta"));
  box.setEditable(true);
  ASSERT_NE(box.getMatchFilterCompleter(), nullptr);
  QKeyEvent press(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, QStringLiteral("a"));
  QApplication::sendEvent(&box, &press);
  EXPECT_GE(box.count(), 2);
}

TEST(PenStyleComboBox, selectsADashStyle) {
  ensureApplication();
  PenStyleComboBox box;
  box.show();
  ASSERT_GT(box.count(), 0);
  box.setCurrentStyle(Qt::DashLine);
  EXPECT_EQ(box.getCurrentStyle(), Qt::DashLine);
  box.show();
  box.repaint();
}

TEST(CurveAxisScaleConfigWidget, absoluteAndRelativeModesWriteTheConfig) {
  ensureApplication();
  CurveAxisScaleConfig config;
  CurveAxisScaleConfigWidget widget;
  widget.setConfig(&config);

  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonAbsoluteToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), CurveAxisScaleConfig::Absolute);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "lineEditAbsoluteMinimumEditingFinished"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "lineEditAbsoluteMaximumEditingFinished"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonRelativeToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), CurveAxisScaleConfig::Relative);
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "lineEditRelativeMinimumEditingFinished"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "lineEditRelativeMaximumEditingFinished"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "radioButtonAutoToggled", Q_ARG(bool, true)));
  EXPECT_EQ(config.getType(), CurveAxisScaleConfig::Auto);

  CurveAxisScaleConfig replacement;
  widget.setConfig(&replacement);
  widget.setConfig(nullptr);
}

TEST(MatchFilterComboBox, completerActivationWritesTheEditText) {
  ensureApplication();
  MatchFilterComboBox box;
  box.addItem(QStringLiteral("alpha"));
  box.addItem(QStringLiteral("beta"));
  box.setEditable(true);
  ASSERT_TRUE(QMetaObject::invokeMethod(&box, "matchFilterCompleterActivated", Q_ARG(QString, QStringLiteral("beta"))));
  EXPECT_EQ(box.currentText(), QStringLiteral("beta"));
  box.setEditText(QStringLiteral("nope"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&box, "lineEditEditingFinished"));
  EXPECT_EQ(box.currentText(), QStringLiteral("nope"));
}

TEST(StatusWidget, cyclesRolesAndFrames) {
  ensureApplication();
  rqt_multiplot::StatusWidget widget;
  QPixmap icon(16, 32);
  icon.fill(Qt::red);
  widget.setIcon(rqt_multiplot::StatusWidget::Error, icon);
  EXPECT_FALSE(widget.getIcon(rqt_multiplot::StatusWidget::Error).isNull());
  widget.setFrames(rqt_multiplot::StatusWidget::Busy, icon, 2, 20.0);
  widget.setCurrentRole(rqt_multiplot::StatusWidget::Busy, QStringLiteral("working"));
  EXPECT_EQ(widget.getCurrentRole(), rqt_multiplot::StatusWidget::Busy);
  widget.pushCurrentRole();
  widget.setCurrentRole(rqt_multiplot::StatusWidget::Okay, QStringLiteral("idle"));
  EXPECT_TRUE(widget.popCurrentRole());
  EXPECT_EQ(widget.getCurrentRole(), rqt_multiplot::StatusWidget::Busy);
  QMetaObject::invokeMethod(&widget, "timerTimeout");
}
