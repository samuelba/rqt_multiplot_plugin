/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PACKAGE_RESOURCE_H
#define RQT_MULTIPLOT_PACKAGE_RESOURCE_H

#include <QAbstractButton>
#include <QAction>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QSvgRenderer>
#include <QVariant>

#include <rqt_multiplot/AmentIndex.h>
#include <rqt_multiplot/Theme.h>

namespace rqt_multiplot {

inline constexpr auto kThemeIconPathProperty = "rqtThemeIconPath";
inline constexpr auto kThemeIconSizeProperty = "rqtThemeIconSize";

inline QString packageShareDirectory() {
  return QString::fromStdString(packageSharePath("rqt_multiplot"));
}

inline QString packageResourcePath(const QString& relativePath) {
  return packageShareDirectory() + "/" + relativePath;
}

inline QPixmap packagePixmap(const QString& relativePath, const QSize& size = QSize(32, 32)) {
  const QString path = packageResourcePath(relativePath);
  if (!QFileInfo::exists(path)) {
    return {};
  }
  if (!path.endsWith(".svg", Qt::CaseInsensitive)) {
    return QPixmap(path);
  }

  QSvgRenderer renderer(path);
  if (!renderer.isValid()) {
    return {};
  }

  QImage image(size, QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  {
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&painter);
  }

  const bool skipTint = relativePath.contains(QLatin1String("status-okay")) || relativePath.contains(QLatin1String("status-error"));
  if (!skipTint) {
    QPainter tintPainter(&image);
    tintPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tintPainter.fillRect(image.rect(), Theme::iconColor());
  }
  return QPixmap::fromImage(image);
}

inline QIcon packageIcon(const QString& relativePath, const QSize& size = QSize(32, 32)) {
  const QPixmap pixmap = packagePixmap(relativePath, size);
  if (pixmap.isNull()) {
    return {};
  }
  return QIcon(pixmap);
}

inline void setThemeIcon(QAbstractButton* button, const QString& relativePath, const QSize& size = QSize(16, 16)) {
  if (button == nullptr) {
    return;
  }
  button->setProperty(kThemeIconPathProperty, relativePath);
  button->setProperty(kThemeIconSizeProperty, QVariant::fromValue(size));
  button->setIcon(packageIcon(relativePath, size));
  button->setIconSize(size);
}

inline void setThemeIcon(QAction* action, const QString& relativePath, const QSize& size = QSize(16, 16)) {
  if (action == nullptr) {
    return;
  }
  action->setProperty(kThemeIconPathProperty, relativePath);
  action->setProperty(kThemeIconSizeProperty, QVariant::fromValue(size));
  action->setIcon(packageIcon(relativePath, size));
}

}  // namespace rqt_multiplot

#endif
