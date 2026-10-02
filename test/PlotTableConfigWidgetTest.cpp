#include <QApplication>
#include <QMetaObject>

#include <gtest/gtest.h>

#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableConfigWidget.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"

namespace {

using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableConfigWidget;
using rqt_multiplot::PlotTableWidget;

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

bool invokeInt(QObject* object, const char* method, int value) {
  return QMetaObject::invokeMethod(object, method, Q_ARG(int, value));
}

bool invokeBool(QObject* object, const char* method, bool value) {
  return QMetaObject::invokeMethod(object, method, Q_ARG(bool, value));
}

}  // namespace

TEST(PlotTableConfigWidget, linkAndPlaybackControlsWriteTheConfig) {
  ensureApplication();
  PlotTableConfig config(nullptr);
  config.splitPlot(config.getPlotConfig(0, 0), Qt::Horizontal);

  PlotTableWidget table;
  table.resize(640, 480);
  table.setConfig(&config);

  PlotTableConfigWidget widget;
  widget.setConfig(&config);
  widget.setPlotTable(&table);
  EXPECT_EQ(widget.getPlotTableWidget(), &table);

  ASSERT_TRUE(invokeInt(&widget, "checkBoxLinkScaleStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_TRUE(config.isScaleLinked());
  ASSERT_TRUE(invokeInt(&widget, "checkBoxLinkCursorStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_TRUE(config.isCursorLinked());
  ASSERT_TRUE(invokeInt(&widget, "checkBoxTrackPointsStateChanged", static_cast<int>(Qt::Checked)));
  EXPECT_TRUE(config.arePointsTracked());
  ASSERT_TRUE(invokeBool(&widget, "pushButtonStartAtZeroToggled", true));
  EXPECT_EQ(config.getTimeAxisFormat(), PlotTableConfig::StartFromZero);
  ASSERT_TRUE(invokeBool(&widget, "pushButtonDateTimeToggled", true));
  EXPECT_EQ(config.getTimeAxisFormat(), PlotTableConfig::DateTime);
  ASSERT_TRUE(invokeBool(&widget, "pushButtonSidebarToggled", true));
  EXPECT_TRUE(config.isSidebarVisible());
  ASSERT_TRUE(invokeBool(&widget, "pushButtonGridToggled", false));
  EXPECT_FALSE(config.isGridVisible());

  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "pushButtonPauseClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "pushButtonClearClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "pushButtonResetLayoutClicked"));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "plotTableJobStarted", Q_ARG(QString, QStringLiteral("loading"))));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "plotTableJobProgressChanged", Q_ARG(double, 0.4)));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "plotTableJobFinished", Q_ARG(QString, QStringLiteral("done"))));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "plotTableJobFailed", Q_ARG(QString, QStringLiteral("failed"))));
  ASSERT_TRUE(QMetaObject::invokeMethod(&widget, "plotTablePlotPausedChanged"));

  widget.setPlotTable(nullptr);
  widget.setConfig(nullptr);
}
