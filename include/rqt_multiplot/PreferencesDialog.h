/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_PREFERENCES_DIALOG_H
#define RQT_MULTIPLOT_PREFERENCES_DIALOG_H

#include <QDialog>
#include <QString>

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

 private:
  Ui::PreferencesDialog* ui_;
  QString timeZoneId_;

  void populateTimeZoneCombo();
  void selectTimeZoneId(const QString& timeZoneId);
  QString selectedTimeZoneId() const;

 private slots:
  void acceptDialog();
};

}  // namespace rqt_multiplot

#endif
