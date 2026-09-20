/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

#include <QApplication>
#include <QCoreApplication>

#include <rclcpp/executors/single_threaded_executor.hpp>
#include <rclcpp/rclcpp.hpp>

#include <rqt_multiplot/LaunchOptions.h>
#include <rqt_multiplot/MultiplotWidget.h>
#include <rqt_multiplot/RosContext.h>
#include <rqt_multiplot/StandaloneWindowSettings.h>

namespace {

void configureQtLogging() {
  const QString rule = QStringLiteral("qt.accessibility.atspi.warning=false");
  const QString existing = qEnvironmentVariable("QT_LOGGING_RULES");
  if (!existing.contains(rule)) {
    const QByteArray value = existing.isEmpty() ? rule.toUtf8() : (existing + QLatin1Char('\n') + rule).toUtf8();
    qputenv("QT_LOGGING_RULES", value);
  }
}

}  // namespace

int main(int argc, char** argv) {
  configureQtLogging();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("multiplot"));
  QApplication::setOrganizationName(QStringLiteral("rqt_multiplot"));

  rqt_multiplot::LaunchOptions options;
  const rqt_multiplot::LaunchParseStatus status = rqt_multiplot::parseLaunchOptions(QCoreApplication::arguments(), options, true);
  if (status == rqt_multiplot::LaunchParseStatus::HelpRequested) {
    rqt_multiplot::printLaunchOptionsHelp(QCoreApplication::arguments());
    return 0;
  }
  if (status == rqt_multiplot::LaunchParseStatus::Error) {
    return 1;
  }

  rclcpp::init(argc, argv);
  auto node = std::make_shared<rclcpp::Node>("multiplot");
  rqt_multiplot::RosContext::setNode(node);

  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node);

  std::atomic<bool> spinning{true};
  std::thread spinThread([&executor, &spinning]() {
    while (spinning.load() && rclcpp::ok()) {
      executor.spin_some();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
  });

  rqt_multiplot::MultiplotWidget widget;
  widget.resize(1280, 800);

  const rqt_multiplot::StandaloneWindowState saved = rqt_multiplot::StandaloneWindowSettings::load();
  if (!saved.geometry.isEmpty()) {
    widget.restoreGeometry(saved.geometry);
  }
  if (saved.maxConfigHistoryLength > 0) {
    widget.setMaxConfigHistoryLength(saved.maxConfigHistoryLength);
  }
  if (!saved.configHistory.isEmpty()) {
    widget.setConfigHistory(saved.configHistory);
  }

  if (!options.configUrl.isEmpty()) {
    widget.loadConfig(options.configUrl);
  }
  if (!options.bagPath.isEmpty()) {
    widget.readBag(options.bagPath);
  }

  widget.show();

  if (options.runAllOnStart) {
    widget.runPlots();
  }

  const int exitCode = QApplication::exec();

  widget.pausePlots();
  spinning.store(false);
  executor.cancel();
  if (spinThread.joinable()) {
    spinThread.join();
  }
  executor.remove_node(node);
  rclcpp::shutdown();

  rqt_multiplot::StandaloneWindowSettings::save(widget, widget.getMaxConfigHistoryLength(), widget.getConfigHistory());

  return exitCode;
}
