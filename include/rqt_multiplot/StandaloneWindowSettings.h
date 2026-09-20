/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_STANDALONE_WINDOW_SETTINGS_H
#define RQT_MULTIPLOT_STANDALONE_WINDOW_SETTINGS_H

#include <QByteArray>
#include <QStringList>

#include <cstddef>

class QWidget;

namespace rqt_multiplot {

struct StandaloneWindowState {
  QByteArray geometry;
  size_t maxConfigHistoryLength = 0;
  QStringList configHistory;
};

class StandaloneWindowSettings {
 public:
  static QString testSettingsFile_;

  static StandaloneWindowState load();
  static void save(const QWidget& widget, size_t maxConfigHistoryLength, const QStringList& configHistory);
};

}  // namespace rqt_multiplot

#endif
