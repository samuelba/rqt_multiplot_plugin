#include <gtest/gtest.h>

#include <QApplication>
#include <QTemporaryDir>
#include <QWidget>

#include <rqt_multiplot/StandaloneWindowSettings.h>

namespace {

using rqt_multiplot::StandaloneWindowSettings;
using rqt_multiplot::StandaloneWindowState;

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

TEST(StandaloneWindowSettings, roundTripsGeometryAndHistory) {
  ensureApplication();
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  StandaloneWindowSettings::testSettingsFile_ = tempDir.filePath(QStringLiteral("multiplot.ini"));

  QWidget widget;
  widget.resize(1024, 768);
  const QByteArray savedGeometry = widget.saveGeometry();
  const QStringList history({QStringLiteral("file:///one.xml"), QStringLiteral("file:///two.xml")});
  StandaloneWindowSettings::save(widget, 8, history);

  const StandaloneWindowState loaded = StandaloneWindowSettings::load();
  EXPECT_EQ(loaded.geometry, savedGeometry);
  EXPECT_EQ(loaded.maxConfigHistoryLength, 8u);
  EXPECT_EQ(loaded.configHistory, history);

  StandaloneWindowSettings::testSettingsFile_.clear();
}
