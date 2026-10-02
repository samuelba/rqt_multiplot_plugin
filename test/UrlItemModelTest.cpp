#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMetaObject>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/FileScheme.hpp"
#include "rqt_multiplot/PackageRegistry.hpp"
#include "rqt_multiplot/PackageScheme.hpp"
#include "rqt_multiplot/UrlItemModel.hpp"

namespace {

using rqt_multiplot::FileScheme;
using rqt_multiplot::PackageRegistry;
using rqt_multiplot::PackageScheme;
using rqt_multiplot::UrlItemModel;

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

TEST(UrlItemModel, fileSchemeListsATempFile) {
  ensureApplication();
  QTemporaryDir tempDir;
  ASSERT_TRUE(tempDir.isValid());
  const QString filePath = tempDir.filePath(QStringLiteral("sample.txt"));
  ASSERT_TRUE(QFile(filePath).open(QIODevice::WriteOnly));

  FileScheme scheme(nullptr, QStringLiteral("file"), tempDir.path(), QDir::Files | QDir::NoDotAndDotDot);
  UrlItemModel model;
  QSignalSpy loaded(&model, &UrlItemModel::urlLoaded);
  model.addScheme(&scheme);

  const QModelIndex schemeIndex = model.index(0, 0, QModelIndex());
  ASSERT_TRUE(schemeIndex.isValid());
  EXPECT_EQ(model.data(schemeIndex, Qt::DisplayRole).toString(), QStringLiteral("file://"));
  ASSERT_EQ(model.rowCount(schemeIndex), 1);
  const QModelIndex rootIndex = model.index(0, 0, schemeIndex);
  ASSERT_TRUE(rootIndex.isValid());
  if (model.rowCount(rootIndex) == 0) {
    ASSERT_TRUE(loaded.wait(3000));
  }
  ASSERT_GT(model.rowCount(rootIndex), 0);
  const QModelIndex fileIndex = model.index(0, 0, rootIndex);
  ASSERT_TRUE(fileIndex.isValid());
  const QString url = UrlItemModel::getUrl(fileIndex);
  const QString path = UrlItemModel::getFilePath(fileIndex);
  EXPECT_TRUE(path.endsWith(QStringLiteral("sample.txt"))) << path.toStdString();
  EXPECT_TRUE(url.contains(QStringLiteral("sample.txt"))) << url.toStdString();
  EXPECT_FALSE(model.getFilePath(url).isEmpty()) << url.toStdString();
}

TEST(UrlItemModel, packageSchemeResolvesTheInstalledPackage) {
  ensureApplication();
  PackageScheme scheme;
  if (scheme.getNumHosts() == 0 && !PackageRegistry::isEmpty()) {
    ASSERT_TRUE(QMetaObject::invokeMethod(&scheme, "registryUpdateFinished"));
  }
  if (scheme.getNumHosts() == 0) {
    QSignalSpy ready(&scheme, &PackageScheme::resetFinished);
    if (PackageRegistry::isEmpty() && !PackageRegistry::isUpdating()) {
      PackageRegistry::update();
    }
    ASSERT_TRUE(ready.wait(5000) || scheme.getNumHosts() > 0);
  }
  ASSERT_GT(scheme.getNumHosts(), 0u);

  const QString share = scheme.getFilePath(QStringLiteral("rqt_multiplot"), QStringLiteral("package.xml"));
  EXPECT_FALSE(share.isEmpty());

  UrlItemModel model;
  model.addScheme(&scheme);
  const QModelIndex schemeIndex = model.index(0, 0, QModelIndex());
  ASSERT_TRUE(schemeIndex.isValid());
  EXPECT_EQ(model.data(schemeIndex, Qt::DisplayRole).toString(), QStringLiteral("package://"));
  ASSERT_GT(model.rowCount(schemeIndex), 0);
  const QModelIndex hostIndex = model.index(0, 0, schemeIndex);
  ASSERT_TRUE(hostIndex.isValid());
  const QString url = UrlItemModel::getUrl(hostIndex);
  EXPECT_TRUE(url.startsWith(QStringLiteral("package://")));
  EXPECT_FALSE(UrlItemModel::getFilePath(hostIndex).isEmpty());
  EXPECT_EQ(UrlItemModel::getScheme(hostIndex), &scheme);

  if (model.rowCount(hostIndex) == 0) {
    QSignalSpy loaded(&model, &UrlItemModel::urlLoaded);
    ASSERT_TRUE(loaded.wait(3000) || model.rowCount(hostIndex) > 0);
  }
  if (model.rowCount(hostIndex) > 0) {
    const QModelIndex pathIndex = model.index(0, 0, hostIndex);
    ASSERT_TRUE(pathIndex.isValid());
    EXPECT_FALSE(UrlItemModel::getFilePath(pathIndex).isEmpty());
    EXPECT_FALSE(model.data(pathIndex, Qt::DisplayRole).toString().isEmpty());
  }
}
