#include <cstdlib>

#include <QApplication>

#include <gtest/gtest.h>

#include <rqt_multiplot/StatusWidget.h>

namespace {

using rqt_multiplot::StatusWidget;

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

TEST(StatusWidget, emitsCurrentRoleChangedWhenTooltipChangesForSameRole) {
  ensureApplication();

  StatusWidget widget;
  int emitCount = 0;
  StatusWidget::Role lastRole = StatusWidget::Okay;
  QObject::connect(&widget, &StatusWidget::currentRoleChanged, [&](StatusWidget::Role role) {
    ++emitCount;
    lastRole = role;
  });

  widget.setCurrentRole(StatusWidget::Error, QStringLiteral("No such message field"));
  ASSERT_EQ(emitCount, 1);
  EXPECT_EQ(lastRole, StatusWidget::Error);
  EXPECT_EQ(widget.toolTip(), QStringLiteral("No such message field"));

  widget.setCurrentRole(StatusWidget::Error, QStringLiteral("Array index or * field must be paired"));
  EXPECT_EQ(emitCount, 2);
  EXPECT_EQ(lastRole, StatusWidget::Error);
  EXPECT_EQ(widget.toolTip(), QStringLiteral("Array index or * field must be paired"));
}

TEST(StatusWidget, doesNotEmitWhenRoleAndTooltipAreUnchanged) {
  ensureApplication();

  StatusWidget widget;
  int emitCount = 0;
  QObject::connect(&widget, &StatusWidget::currentRoleChanged, [&](StatusWidget::Role) { ++emitCount; });

  widget.setCurrentRole(StatusWidget::Error, QStringLiteral("No topic selected"));
  ASSERT_EQ(emitCount, 1);

  widget.setCurrentRole(StatusWidget::Error, QStringLiteral("No topic selected"));
  EXPECT_EQ(emitCount, 1);
}

}  // namespace
