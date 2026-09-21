#include <cstdlib>

#include <QApplication>
#include <QString>

#include <gtest/gtest.h>

#include "rqt_multiplot/UrlComboBox.hpp"

namespace {

using rqt_multiplot::UrlComboBox;

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

TEST(UrlComboBox, emitsCurrentUrlChangedWhenIndexChanges) {
  ensureApplication();

  UrlComboBox box;
  box.addItem(QStringLiteral("file:///tmp/a.xml"));
  box.addItem(QStringLiteral("file:///tmp/b.xml"));

  int emitCount = 0;
  QString lastUrl;
  QObject::connect(&box, &UrlComboBox::currentUrlChanged, [&](const QString& url) {
    ++emitCount;
    lastUrl = url;
  });

  box.setCurrentIndex(1);

  EXPECT_EQ(emitCount, 1);
  EXPECT_EQ(lastUrl, QStringLiteral("file:///tmp/b.xml"));
  EXPECT_EQ(box.getCurrentUrl(), QStringLiteral("file:///tmp/b.xml"));
}

TEST(UrlComboBox, setCurrentUrlUpdatesFromExistingItem) {
  ensureApplication();

  UrlComboBox box;
  box.addItem(QStringLiteral("file:///tmp/a.xml"));
  box.addItem(QStringLiteral("file:///tmp/b.xml"));

  box.setCurrentUrl(QStringLiteral("file:///tmp/b.xml"));

  EXPECT_EQ(box.getCurrentUrl(), QStringLiteral("file:///tmp/b.xml"));
  EXPECT_EQ(box.currentIndex(), 1);
}

}  // namespace
