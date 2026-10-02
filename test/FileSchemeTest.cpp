#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/FileScheme.hpp"

namespace {

using rqt_multiplot::FileScheme;

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

bool waitUntilLoaded(FileScheme& scheme, const QModelIndex& parent) {
  if (scheme.getNumPaths(QModelIndex(), parent) > 0) {
    return true;
  }
  QSignalSpy spy(&scheme, &FileScheme::pathLoaded);
  return spy.wait(3000) && scheme.getNumPaths(QModelIndex(), parent) > 0;
}

}  // namespace

TEST(FileScheme, listsFilesUnderRootAndResolvesPaths) {
  ensureApplication();
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  ASSERT_TRUE(QDir(tempDir.path()).mkpath(QStringLiteral("nested")));
  QFile sample(tempDir.filePath(QStringLiteral("sample.txt")));
  ASSERT_TRUE(sample.open(QIODevice::WriteOnly));
  ASSERT_EQ(sample.write("x"), 1);
  sample.close();
  QFile nested(tempDir.filePath(QStringLiteral("nested/inner.txt")));
  ASSERT_TRUE(nested.open(QIODevice::WriteOnly));
  nested.close();

  FileScheme scheme(nullptr, QStringLiteral("file"), tempDir.path(), QDir::AllEntries | QDir::NoDotAndDotDot);
  EXPECT_EQ(scheme.getPrefix(), QStringLiteral("file"));
  EXPECT_EQ(scheme.getRootPath(), tempDir.path());
  EXPECT_EQ(scheme.getFilter(), QDir::AllEntries | QDir::NoDotAndDotDot);
  scheme.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
  EXPECT_EQ(scheme.getFilter(), QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

  EXPECT_EQ(scheme.getNumHosts(), 0u);
  EXPECT_FALSE(scheme.getHostIndex(0).isValid());
  EXPECT_FALSE(scheme.getHostData(QModelIndex(), Qt::DisplayRole).isValid());
  EXPECT_TRUE(scheme.getHost(QModelIndex()).isEmpty());
  EXPECT_EQ(scheme.getNumPaths(QModelIndex(), QModelIndex()), 1u);

  const QModelIndex root = scheme.getPathIndex(QModelIndex(), 0, QModelIndex());
  ASSERT_TRUE(root.isValid());
  EXPECT_EQ(scheme.getPathData(root, Qt::DisplayRole).toString(), QStringLiteral("/"));
  EXPECT_FALSE(scheme.getPathData(root, Qt::DecorationRole).isValid());
  EXPECT_TRUE(scheme.getFilePath(QModelIndex(), QModelIndex()).isEmpty());
  EXPECT_EQ(scheme.getFilePath(QString(), QStringLiteral("sample.txt")), tempDir.filePath(QStringLiteral("sample.txt")));

  ASSERT_TRUE(waitUntilLoaded(scheme, root));
  const size_t childCount = scheme.getNumPaths(QModelIndex(), root);
  ASSERT_GE(childCount, 1u);

  bool foundSample = false;
  QModelIndex nestedIndex;
  for (size_t row = 0; row < childCount; ++row) {
    const QModelIndex child = scheme.getPathIndex(QModelIndex(), row, root);
    const QString name = scheme.getPathData(child, Qt::DisplayRole).toString();
    if (name == QStringLiteral("sample.txt")) {
      foundSample = true;
      EXPECT_EQ(scheme.getFilePath(QModelIndex(), child), tempDir.filePath(QStringLiteral("sample.txt")));
      EXPECT_EQ(scheme.getPath(QModelIndex(), child), QStringLiteral("sample.txt"));
    }
    if (name == QStringLiteral("nested")) {
      nestedIndex = child;
    }
  }
  EXPECT_TRUE(foundSample);
  ASSERT_TRUE(nestedIndex.isValid());
  ASSERT_TRUE(waitUntilLoaded(scheme, nestedIndex));
  EXPECT_GE(scheme.getNumPaths(QModelIndex(), nestedIndex), 1u);

  scheme.setRootPath(tempDir.filePath(QStringLiteral("nested")));
  EXPECT_EQ(scheme.getRootPath(), tempDir.filePath(QStringLiteral("nested")));
}
