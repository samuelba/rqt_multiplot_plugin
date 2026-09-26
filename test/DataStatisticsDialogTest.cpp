#include <QApplication>
#include <QPointF>
#include <QRadioButton>
#include <QTableWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/BoundingRectangle.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/DataStatisticsDialog.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::BoundingRectangle;
using rqt_multiplot::DataStatisticsDialog;
using rqt_multiplot::PlotConfig;
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

QString cellText(const QTableWidget* table, int row, int column) {
  const QTableWidgetItem* item = table->item(row, column);
  if (item == nullptr) {
    return {};
  }
  return item->text();
}

TEST(DataStatisticsDialog, visibleWindowAndAxisChangeSummaries) {
  ensureApplication();

  auto* config = new PlotConfig();
  config->addCurve()->setTitle(QStringLiteral("alpha"));
  config->addCurve()->setTitle(QStringLiteral("beta"));

  auto* widget = new PlotWidget();
  widget->setConfig(config);
  widget->getCurves().at(0)->getData()->appendPoint(QPointF(0.0, 0.0));
  widget->getCurves().at(0)->getData()->appendPoint(QPointF(1.0, 10.0));
  widget->getCurves().at(0)->getData()->appendPoint(QPointF(2.0, 20.0));
  widget->getCurves().at(1)->getData()->appendPoint(QPointF(0.0, 1.0));
  widget->getCurves().at(1)->getData()->appendPoint(QPointF(1.0, 1.0));
  widget->getCurves().at(1)->getData()->appendPoint(QPointF(5.0, 100.0));
  widget->forceReplot();
  widget->setCurrentScale(BoundingRectangle(QPointF(0.0, 0.0), QPointF(2.0, 20.0)));

  const BoundingRectangle scale = widget->getCurrentScale();
  ASSERT_DOUBLE_EQ(scale.getMinimum().x(), 0.0);
  ASSERT_DOUBLE_EQ(scale.getMaximum().x(), 2.0);
  ASSERT_DOUBLE_EQ(scale.getMinimum().y(), 0.0);
  ASSERT_DOUBLE_EQ(scale.getMaximum().y(), 20.0);

  DataStatisticsDialog dialog;
  dialog.setPlot(widget);

  auto* table = dialog.findChild<QTableWidget*>(QStringLiteral("dataStatisticsTable"));
  ASSERT_NE(table, nullptr);
  ASSERT_EQ(table->rowCount(), 2);
  EXPECT_EQ(cellText(table, 0, 0), QStringLiteral("alpha"));
  EXPECT_EQ(cellText(table, 1, 0), QStringLiteral("beta"));
  EXPECT_EQ(cellText(table, 0, 2), QString::number(10.0, 'g', 8));
  EXPECT_EQ(cellText(table, 1, 2), QString::number(1.0, 'g', 8));

  auto* allPoints = dialog.findChild<QRadioButton*>(QStringLiteral("radioDataStatisticsAll"));
  ASSERT_NE(allPoints, nullptr);
  allPoints->setChecked(true);
  EXPECT_EQ(cellText(table, 0, 2), QString::number(10.0, 'g', 8));
  EXPECT_EQ(cellText(table, 1, 2), QString::number(34.0, 'g', 8));

  auto* xValues = dialog.findChild<QRadioButton*>(QStringLiteral("radioDataStatisticsAxisX"));
  ASSERT_NE(xValues, nullptr);
  xValues->setChecked(true);
  EXPECT_EQ(cellText(table, 0, 2), QString::number(1.0, 'g', 8));
  EXPECT_EQ(cellText(table, 1, 2), QString::number(2.0, 'g', 8));

  auto* visiblePoints = dialog.findChild<QRadioButton*>(QStringLiteral("radioDataStatisticsVisible"));
  ASSERT_NE(visiblePoints, nullptr);
  visiblePoints->setChecked(true);
  EXPECT_EQ(cellText(table, 0, 2), QString::number(1.0, 'g', 8));
  EXPECT_EQ(cellText(table, 1, 2), QString::number(0.5, 'g', 8));

  delete widget->getConfig();
  delete widget;
}

}  // namespace
