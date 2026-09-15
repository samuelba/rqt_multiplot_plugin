#include <cstdlib>

#include <QApplication>
#include <QMetaObject>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include <gtest/gtest.h>
#include <qwt/qwt_plot.h>

#include <rqt_multiplot/CurveAxisConfig.h>
#include <rqt_multiplot/CurveConfig.h>
#include <rqt_multiplot/CurveData.h>
#include <rqt_multiplot/PlotConfig.h>
#include <rqt_multiplot/PlotCurve.h>
#include <rqt_multiplot/PlotWidget.h>

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotCurve;
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

QStringList* gQtWarnings = nullptr;

void captureQtWarnings(QtMsgType type, const QMessageLogContext& /*context*/, const QString& message) {
  if ((gQtWarnings != nullptr) && (type == QtWarningMsg)) {
    gQtWarnings->append(message);
  }
}

void configureSnapshotCurve(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setTopic("/array");
  config->getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config->getAxisConfig(CurveConfig::Y)->setField("position/*");
  config->getStyleConfig()->setFadeHistory(3);
}

TEST(PlotCurve, destroyingParentAfterSamplesDoesNotCrash) {
  ensureApplication();

  auto* parent = new QWidget();
  auto* plot = new QwtPlot(parent);
  auto* curve = new PlotCurve(parent);
  curve->attach(plot);
  curve->getData()->appendPoint(QPointF(1.0, 2.0));
  curve->getData()->appendPoint(QPointF(2.0, 3.0));

  delete parent;
}

TEST(PlotCurve, destroyingParentAfterSnapshotGhostsDoesNotCrash) {
  ensureApplication();

  CurveConfig config;
  configureSnapshotCurve(&config);

  auto* parent = new QWidget();
  auto* plot = new QwtPlot(parent);
  auto* curve = new PlotCurve(parent);
  curve->attach(plot);
  curve->setConfig(&config);
  curve->run();

  const QVector<QPointF> first{QPointF(0.0, 1.0), QPointF(1.0, 2.0)};
  const QVector<QPointF> second{QPointF(0.0, 3.0), QPointF(1.0, 4.0)};
  ASSERT_TRUE(QMetaObject::invokeMethod(curve, "dataSequencerSeriesReceived", Qt::DirectConnection, Q_ARG(QVector<QPointF>, first)));
  ASSERT_TRUE(QMetaObject::invokeMethod(curve, "dataSequencerSeriesReceived", Qt::DirectConnection, Q_ARG(QVector<QPointF>, second)));
  ASSERT_EQ(plot->itemList().count(), 3);

  delete parent;
}

TEST(PlotCurve, clearingConfigDoesNotWarnAboutMissingChangedSignal) {
  ensureApplication();

  QStringList warnings;
  gQtWarnings = &warnings;
  const QtMessageHandler previous = qInstallMessageHandler(captureQtWarnings);

  CurveConfig config;
  PlotCurve curve;
  curve.setConfig(&config);
  curve.setConfig(nullptr);

  qInstallMessageHandler(previous);
  gQtWarnings = nullptr;

  for (const QString& warning : warnings) {
    EXPECT_FALSE(warning.contains(QStringLiteral("No such signal"))) << warning.toStdString();
  }
}

TEST(PlotWidget, destroyingAfterPlottedSamplesDoesNotCrash) {
  ensureApplication();

  PlotConfig config;
  config.addCurve();

  auto* widget = new PlotWidget();
  widget->setConfig(&config);
  const auto curves = widget->findChildren<PlotCurve*>();
  ASSERT_EQ(curves.size(), 1);
  curves.front()->getData()->appendPoint(QPointF(1.0, 2.0));
  widget->forceReplot();

  delete widget;
}

}  // namespace
