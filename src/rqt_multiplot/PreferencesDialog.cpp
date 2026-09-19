/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/PreferencesDialog.h"

#include "rqt_multiplot/Theme.h"
#include "rqt_multiplot/TimeZoneUtil.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QTimeZone>

#include <ui_PreferencesDialog.h>

namespace rqt_multiplot {

namespace {
constexpr auto kTimeZoneLocal = "local";
constexpr auto kTimeZoneUtc = "utc";
constexpr auto kTimeZoneRole = Qt::UserRole;
}  // namespace

PreferencesDialog::PreferencesDialog(QWidget* parent, Qt::WindowFlags flags)
    : QDialog(parent, flags),
      ui_(new Ui::PreferencesDialog()),
      timeZoneId_(QString::fromLatin1(kTimeZoneLocal)),
      themeId_(QString::fromLatin1(Theme::kLightId)) {
  ui_->setupUi(this);
  populateTimeZoneCombo();
  populateThemeCombo();
  Theme::apply(this);
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::acceptDialog);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);
}

PreferencesDialog::~PreferencesDialog() {
  delete ui_;
}

void PreferencesDialog::setTimeZoneId(const QString& timeZoneId) {
  timeZoneId_ = timeZoneId;
  selectTimeZoneId(timeZoneId);
}

QString PreferencesDialog::timeZoneId() const {
  return timeZoneId_;
}

void PreferencesDialog::setThemeId(const QString& themeId) {
  themeId_ = Theme::toId(Theme::fromId(themeId));
  selectThemeId(themeId_);
}

QString PreferencesDialog::themeId() const {
  return themeId_;
}

void PreferencesDialog::populateTimeZoneCombo() {
  QComboBox* combo = ui_->comboTimeZone;
  combo->clear();
  const QString localLabel =
      QStringLiteral("Local (%1)")
          .arg(TimeZoneUtil::localTimeZoneId().isEmpty() ? QStringLiteral("system") : TimeZoneUtil::localTimeZoneId());
  combo->addItem(localLabel, QString::fromLatin1(kTimeZoneLocal));
  combo->addItem(QStringLiteral("UTC"), QString::fromLatin1(kTimeZoneUtc));

  const QList<QByteArray> zoneIds = QTimeZone::availableTimeZoneIds();
  for (const QByteArray& zoneId : zoneIds) {
    const QString id = QString::fromUtf8(zoneId);
    if (id == QLatin1String(kTimeZoneUtc)) {
      continue;
    }
    combo->addItem(id, id);
  }

  combo->setEditable(true);
}

void PreferencesDialog::populateThemeCombo() {
  QComboBox* combo = ui_->comboTheme;
  combo->clear();
  combo->addItem(QStringLiteral("Light"), QString::fromLatin1(Theme::kLightId));
  combo->addItem(QStringLiteral("Dark"), QString::fromLatin1(Theme::kDarkId));
  selectThemeId(themeId_);
}

void PreferencesDialog::selectTimeZoneId(const QString& timeZoneId) {
  QComboBox* combo = ui_->comboTimeZone;
  const int index = combo->findData(timeZoneId, kTimeZoneRole);
  if (index >= 0) {
    combo->setCurrentIndex(index);
    return;
  }

  combo->setCurrentText(timeZoneId);
}

void PreferencesDialog::selectThemeId(const QString& themeId) {
  QComboBox* combo = ui_->comboTheme;
  const int index = combo->findData(themeId, kTimeZoneRole);
  if (index >= 0) {
    combo->setCurrentIndex(index);
    return;
  }

  combo->setCurrentIndex(0);
}

QString PreferencesDialog::selectedTimeZoneId() const {
  const QComboBox* combo = ui_->comboTimeZone;
  const QVariant data = combo->currentData(kTimeZoneRole);
  if (data.isValid()) {
    return data.toString();
  }

  return combo->currentText().trimmed();
}

QString PreferencesDialog::selectedThemeId() const {
  const QVariant data = ui_->comboTheme->currentData(kTimeZoneRole);
  if (data.isValid()) {
    return Theme::toId(Theme::fromId(data.toString()));
  }

  return QString::fromLatin1(Theme::kLightId);
}

void PreferencesDialog::acceptDialog() {
  timeZoneId_ = selectedTimeZoneId();
  themeId_ = selectedThemeId();
  accept();
}

}  // namespace rqt_multiplot
