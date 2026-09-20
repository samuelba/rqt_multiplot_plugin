/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PREFERENCES_DIALOG_H
#define RQT_MULTIPLOT_PREFERENCES_DIALOG_H

#include <QDialog>
#include <QString>

#include <rqt_multiplot/PlotTitleStyle.h>

namespace Ui {
class PreferencesDialog;
}

namespace rqt_multiplot {

class PreferencesDialog : public QDialog {
  Q_OBJECT
 public:
  explicit PreferencesDialog(QWidget* parent = nullptr, Qt::WindowFlags flags = {});
  ~PreferencesDialog() override;

  void setTimeZoneId(const QString& timeZoneId);
  QString timeZoneId() const;
  void setThemeId(const QString& themeId);
  QString themeId() const;
  void setOpenGLCanvasEnabled(bool enabled);
  bool isOpenGLCanvasEnabled() const;
  void setPlotTitleStyle(const PlotTitleStyle& style);
  PlotTitleStyle plotTitleStyle() const;
  void setOverrideActive(bool active);
  bool isOverrideActive() const;

 signals:
  void saveAsDefaultsRequested();
  void overrideInConfigurationRequested();
  void clearConfigurationOverrideRequested();
  void restoreFactoryDefaultsRequested();

 protected:
  bool eventFilter(QObject* object, QEvent* event) override;

 private:
  Ui::PreferencesDialog* ui_;
  QString timeZoneId_;
  QString themeId_;
  bool openGLCanvasEnabled_;
  PlotTitleStyle plotTitleStyle_;
  bool overrideActive_;

  void populateTimeZoneCombo();
  void populateThemeCombo();
  void selectTimeZoneId(const QString& timeZoneId);
  void selectThemeId(const QString& themeId);
  QString selectedTimeZoneId() const;
  QString selectedThemeId() const;
  void updateStatus();
  void syncFromWidgets();
  void updatePlotTitleColorSwatch();
  void updatePlotTitlePreview();
  PlotTitleStyle selectedPlotTitleStyle() const;

 private slots:
  void acceptDialog();
  void plotTitleSettingsChanged();
};

}  // namespace rqt_multiplot

#endif
