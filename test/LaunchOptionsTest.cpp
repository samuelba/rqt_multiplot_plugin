#include <QApplication>

#include <gtest/gtest.h>

#include <rqt_multiplot/LaunchOptions.h>

namespace {

using rqt_multiplot::LaunchOptions;
using rqt_multiplot::LaunchParseStatus;
using rqt_multiplot::parseLaunchOptions;

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

TEST(LaunchOptions, parsesEmptyArguments) {
  LaunchOptions options;
  EXPECT_EQ(parseLaunchOptions(QStringList{QStringLiteral("multiplot")}, options), LaunchParseStatus::Ok);
  EXPECT_TRUE(options.configUrl.isEmpty());
  EXPECT_TRUE(options.bagPath.isEmpty());
  EXPECT_FALSE(options.runAllOnStart);
}

TEST(LaunchOptions, parsesConfigBagAndRunAllFlags) {
  LaunchOptions options;
  EXPECT_EQ(parseLaunchOptions(QStringList{QStringLiteral("multiplot"), QStringLiteral("-c"), QStringLiteral("/tmp/layout.xml"),
                                           QStringLiteral("--multiplot-bag"), QStringLiteral("/tmp/bag"), QStringLiteral("-r")},
                               options),
            LaunchParseStatus::Ok);
  EXPECT_EQ(options.configUrl, QStringLiteral("file:///tmp/layout.xml"));
  EXPECT_EQ(options.bagPath, QStringLiteral("/tmp/bag"));
  EXPECT_TRUE(options.runAllOnStart);
}

TEST(LaunchOptions, parsesPluginStyleArgumentsWithoutProgramName) {
  LaunchOptions options;
  EXPECT_EQ(parseLaunchOptions(QStringList{QStringLiteral("--multiplot-config"), QStringLiteral("file:///cfg.xml")}, options),
            LaunchParseStatus::Ok);
  EXPECT_EQ(options.configUrl, QStringLiteral("file:///cfg.xml"));
}

TEST(LaunchOptions, helpRequestedWhenProcessHelpEnabled) {
  ensureApplication();
  LaunchOptions options;
  EXPECT_EQ(parseLaunchOptions(QStringList{QStringLiteral("multiplot"), QStringLiteral("--help")}, options, true),
            LaunchParseStatus::HelpRequested);
}
