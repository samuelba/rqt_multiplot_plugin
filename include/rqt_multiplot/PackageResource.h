/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PACKAGE_RESOURCE_H
#define RQT_MULTIPLOT_PACKAGE_RESOURCE_H

#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QSvgRenderer>

#include <rqt_multiplot/AmentIndex.h>

namespace rqt_multiplot {

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
  return QPixmap::fromImage(image);
}

inline QIcon packageIcon(const QString& relativePath, const QSize& size = QSize(32, 32)) {
  const QPixmap pixmap = packagePixmap(relativePath, size);
  if (pixmap.isNull()) {
    return {};
  }
  return QIcon(pixmap);
}

}  // namespace rqt_multiplot

#endif
