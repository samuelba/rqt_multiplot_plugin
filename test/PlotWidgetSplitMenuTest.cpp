#include <cstdlib>

#include <QApplication>
#include <QGridLayout>
#include <QLineEdit>
#include <QList>
#include <QPair>
#include <QPushButton>
#include <QSpacerItem>
#include <QToolButton>
#include <QWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::PlotWidget;

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

TEST(PlotWidget, splitMenuShowsIconGridWithTooltips) {
  ensureApplication();

  PlotWidget widget;

  auto* splitButton = widget.findChild<QPushButton*>("pushButtonSplit");
  ASSERT_NE(splitButton, nullptr);
  EXPECT_FALSE(splitButton->icon().isNull());

  auto* grid = widget.findChild<QWidget*>("splitDirectionGrid");
  ASSERT_NE(grid, nullptr);
  auto* layout = qobject_cast<QGridLayout*>(grid->layout());
  ASSERT_NE(layout, nullptr);
  EXPECT_EQ(layout->rowCount(), 2);
  EXPECT_EQ(layout->columnCount(), 2);

  auto* left = widget.findChild<QToolButton*>("toolButtonSplitLeft");
  auto* right = widget.findChild<QToolButton*>("toolButtonSplitRight");
  auto* up = widget.findChild<QToolButton*>("toolButtonSplitUp");
  auto* down = widget.findChild<QToolButton*>("toolButtonSplitDown");
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  ASSERT_NE(up, nullptr);
  ASSERT_NE(down, nullptr);

  auto* itemLeft = layout->itemAtPosition(0, 0);
  auto* itemRight = layout->itemAtPosition(0, 1);
  auto* itemUp = layout->itemAtPosition(1, 0);
  auto* itemDown = layout->itemAtPosition(1, 1);
  ASSERT_NE(itemLeft, nullptr);
  ASSERT_NE(itemRight, nullptr);
  ASSERT_NE(itemUp, nullptr);
  ASSERT_NE(itemDown, nullptr);
  EXPECT_EQ(itemLeft->widget(), left);
  EXPECT_EQ(itemRight->widget(), right);
  EXPECT_EQ(itemUp->widget(), up);
  EXPECT_EQ(itemDown->widget(), down);

  EXPECT_EQ(left->text(), QString());
  EXPECT_EQ(left->toolTip(), QString("Split left"));
  EXPECT_EQ(right->toolTip(), QString("Split right"));
  EXPECT_EQ(up->toolTip(), QString("Split up"));
  EXPECT_EQ(down->toolTip(), QString("Split down"));
  EXPECT_FALSE(left->icon().isNull());
  EXPECT_FALSE(right->icon().isNull());
  EXPECT_FALSE(up->icon().isNull());
  EXPECT_FALSE(down->icon().isNull());
}

TEST(PlotWidget, splitMenuButtonsRequestMatchingSplits) {
  ensureApplication();

  PlotWidget widget;
  QList<QPair<Qt::Orientation, bool>> requests;
  QObject::connect(&widget, &PlotWidget::splitRequested,
                   [&](Qt::Orientation orientation, bool insertBefore) { requests.append({orientation, insertBefore}); });

  auto* left = widget.findChild<QToolButton*>("toolButtonSplitLeft");
  auto* right = widget.findChild<QToolButton*>("toolButtonSplitRight");
  auto* up = widget.findChild<QToolButton*>("toolButtonSplitUp");
  auto* down = widget.findChild<QToolButton*>("toolButtonSplitDown");
  ASSERT_NE(left, nullptr);
  ASSERT_NE(right, nullptr);
  ASSERT_NE(up, nullptr);
  ASSERT_NE(down, nullptr);

  left->click();
  right->click();
  up->click();
  down->click();

  ASSERT_EQ(requests.count(), 4);
  EXPECT_EQ(requests.at(0), (QPair<Qt::Orientation, bool>{Qt::Horizontal, true}));
  EXPECT_EQ(requests.at(1), (QPair<Qt::Orientation, bool>{Qt::Horizontal, false}));
  EXPECT_EQ(requests.at(2), (QPair<Qt::Orientation, bool>{Qt::Vertical, true}));
  EXPECT_EQ(requests.at(3), (QPair<Qt::Orientation, bool>{Qt::Vertical, false}));
}

TEST(PlotWidget, titleIsLeftAlignedWithRoomForControls) {
  ensureApplication();

  PlotWidget widget;
  auto* title = widget.findChild<QLineEdit*>("lineEditTitle");
  ASSERT_NE(title, nullptr);
  EXPECT_TRUE(title->alignment().testFlag(Qt::AlignLeft));
  EXPECT_FALSE(title->alignment().testFlag(Qt::AlignHCenter));

  auto* grid = qobject_cast<QGridLayout*>(widget.layout());
  ASSERT_NE(grid, nullptr);

  int titleColumn = -1;
  for (int index = 0; index < grid->count(); ++index) {
    int row = 0;
    int column = 0;
    int rowSpan = 0;
    int columnSpan = 0;
    grid->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
    if ((row == 0) && (grid->itemAt(index)->widget() == title)) {
      titleColumn = column;
      break;
    }
  }
  ASSERT_GE(titleColumn, 0);

  for (int index = 0; index < grid->count(); ++index) {
    int row = 0;
    int column = 0;
    int rowSpan = 0;
    int columnSpan = 0;
    grid->getItemPosition(index, &row, &column, &rowSpan, &columnSpan);
    if ((row != 0) || (column >= titleColumn)) {
      continue;
    }

    QSpacerItem* spacer = grid->itemAt(index)->spacerItem();
    if (spacer != nullptr) {
      EXPECT_FALSE(spacer->expandingDirections().testFlag(Qt::Horizontal));
    }
  }
}

}  // namespace
