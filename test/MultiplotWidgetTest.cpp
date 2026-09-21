#include <cstdlib>

#include <QAbstractButton>
#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointF>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>
#include <QWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/MultiplotWidget.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/XmlSettings.hpp"

namespace {

using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::MultiplotWidget;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::XmlSettings;

bool windowTitleShowsModified(const MultiplotWidget& widget) {
  return widget.windowTitle().endsWith(QLatin1Char('*'));
}

QString writeTwoPlotConfig(const QTemporaryDir& dir) {
  const QString path = dir.filePath(QStringLiteral("loaded.xml"));
  MultiplotConfig config(nullptr);
  config.getTableConfig(0)->setTitle(QStringLiteral("Loaded"));
  config.getTableConfig(0)->setNumPlots(2, 1);

  QSettings settings(path, XmlSettings::format);
  settings.beginGroup("rqt_multiplot");
  config.save(settings);
  settings.endGroup();
  settings.sync();
  return path;
}

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

void sendLeftClick(QWidget* target) {
  const QPointF localPos = target->rect().center();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMouseEvent press(QEvent::MouseButtonPress, localPos, target->mapToGlobal(localPos.toPoint()), Qt::LeftButton, Qt::LeftButton,
                    Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease, localPos, target->mapToGlobal(localPos.toPoint()), Qt::LeftButton, Qt::NoButton,
                      Qt::NoModifier);
#else
  QMouseEvent press(QEvent::MouseButtonPress, localPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QMouseEvent release(QEvent::MouseButtonRelease, localPos, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
#endif
  QApplication::sendEvent(target, &press);
  QApplication::sendEvent(target, &release);
}

QDockWidget* makeDockedWidget(MultiplotWidget** widgetOut, QPushButton** closeButtonOut) {
  auto* dock = new QDockWidget();
  auto* titleBar = new QWidget();
  auto* closeButton = new QPushButton(QStringLiteral("X"), titleBar);
  closeButton->setObjectName(QStringLiteral("close_button"));
  auto* layout = new QHBoxLayout(titleBar);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(closeButton);
  dock->setTitleBarWidget(titleBar);

  auto* widget = new MultiplotWidget();
  dock->setWidget(widget);
  dock->resize(640, 480);
  dock->show();
  widget->show();
  QApplication::processEvents();

  *widgetOut = widget;
  *closeButtonOut = closeButton;
  return dock;
}

TEST(MultiplotWidget, startsUnmodifiedAfterShow) {
  ensureApplication();

  MultiplotWidget widget;
  widget.resize(640, 480);
  widget.show();
  QApplication::processEvents();

  EXPECT_FALSE(windowTitleShowsModified(widget));
}

TEST(MultiplotWidget, staysUnmodifiedAfterLoadingConfig) {
  ensureApplication();

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = writeTwoPlotConfig(dir);
  const QString url = QStringLiteral("file://") + path;

  MultiplotWidget widget;
  widget.resize(640, 480);
  widget.show();
  widget.loadConfig(url);
  QApplication::processEvents();

  EXPECT_TRUE(widget.windowTitle().contains(url));
  EXPECT_FALSE(windowTitleShowsModified(widget));
}

TEST(MultiplotWidget, loadsFirstHistoryEntryWhenNoConfigLoaded) {
  ensureApplication();

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = writeTwoPlotConfig(dir);
  const QString url = QStringLiteral("file://") + path;

  MultiplotWidget widget;
  widget.resize(640, 480);
  widget.show();
  QApplication::processEvents();
  ASSERT_FALSE(widget.windowTitle().contains(url));

  widget.setConfigHistory(QStringList() << url);
  QApplication::processEvents();

  EXPECT_TRUE(widget.windowTitle().contains(url));
  EXPECT_EQ(widget.getConfig()->getTableConfig(0)->getTitle(), QStringLiteral("Loaded"));
  EXPECT_FALSE(windowTitleShowsModified(widget));
}

TEST(MultiplotWidget, staysUnmodifiedAfterHistoryRestore) {
  ensureApplication();

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = writeTwoPlotConfig(dir);
  const QString url = QStringLiteral("file://") + path;

  MultiplotWidget widget;
  widget.resize(640, 480);
  widget.show();
  widget.loadConfig(url);
  QApplication::processEvents();
  ASSERT_FALSE(windowTitleShowsModified(widget));

  widget.setConfigHistory(QStringList() << url << QStringLiteral("file:///tmp/other.xml"));
  QApplication::processEvents();

  EXPECT_TRUE(widget.windowTitle().contains(url));
  EXPECT_FALSE(windowTitleShowsModified(widget));
}

TEST(MultiplotWidget, marksModifiedAfterLoadWhenTitleChanges) {
  ensureApplication();

  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString url = QStringLiteral("file://") + writeTwoPlotConfig(dir);

  MultiplotWidget widget;
  widget.show();
  widget.loadConfig(url);
  QApplication::processEvents();
  ASSERT_FALSE(windowTitleShowsModified(widget));

  widget.getConfig()->getTableConfig(0)->setTitle(QStringLiteral("Changed"));
  EXPECT_TRUE(windowTitleShowsModified(widget));
}

TEST(MultiplotWidget, confirmCloseReturnsTrueWhenUnmodified) {
  ensureApplication();

  MultiplotWidget widget;
  EXPECT_FALSE(widget.getConfig()->getTableConfig(0) == nullptr);
  EXPECT_TRUE(widget.confirmClose());
}

TEST(MultiplotWidget, savePromptAppearsWhileDockIsVisible) {
  ensureApplication();

  MultiplotWidget* widget = nullptr;
  QPushButton* closeButton = nullptr;
  QDockWidget* dock = makeDockedWidget(&widget, &closeButton);
  ASSERT_NE(widget, nullptr);
  ASSERT_NE(closeButton, nullptr);

  PlotTableConfig* table = widget->getConfig()->getTableConfig(0);
  ASSERT_NE(table, nullptr);
  table->setTitle(QStringLiteral("Motors"));

  bool dockVisibleDuringPrompt = false;
  bool widgetVisibleDuringPrompt = false;
  QMessageBox* prompt = nullptr;
  QTimer::singleShot(0, [&]() {
    prompt = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
    dockVisibleDuringPrompt = dock->isVisible();
    widgetVisibleDuringPrompt = widget->isVisible();
    if (prompt != nullptr) {
      if (QAbstractButton* discard = prompt->button(QMessageBox::Discard)) {
        discard->click();
      } else {
        prompt->reject();
      }
    }
  });

  sendLeftClick(closeButton);

  EXPECT_NE(prompt, nullptr);
  EXPECT_TRUE(dockVisibleDuringPrompt);
  EXPECT_TRUE(widgetVisibleDuringPrompt);

  delete dock;
}

}  // namespace
