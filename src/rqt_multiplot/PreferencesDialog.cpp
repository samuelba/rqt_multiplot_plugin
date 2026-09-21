/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/PreferencesDialog.hpp"

#include "rqt_multiplot/Theme.hpp"
#include "rqt_multiplot/TimeZoneUtil.hpp"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
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
      themeId_(QString::fromLatin1(Theme::kLightId)),
      openGLCanvasEnabled_(false),
      plotTitleStyle_(PlotTitleStyle::factory()),
      overrideActive_(false) {
  ui_->setupUi(this);
  populateTimeZoneCombo();
  populateThemeCombo();
  ui_->checkOpenGLCanvas->setChecked(false);
  ui_->labelPlotTitleColorSwatch->setAutoFillBackground(true);
  ui_->labelPlotTitleColorSwatch->installEventFilter(this);
  setPlotTitleStyle(plotTitleStyle_);
  Theme::apply(this);
  connect(ui_->buttonBox, &QDialogButtonBox::accepted, this, &PreferencesDialog::acceptDialog);
  connect(ui_->buttonBox, &QDialogButtonBox::rejected, this, &PreferencesDialog::reject);
  connect(ui_->buttonSaveAsDefaults, &QPushButton::clicked, this, [this]() {
    syncFromWidgets();
    emit saveAsDefaultsRequested();
  });
  connect(ui_->buttonOverrideConfiguration, &QPushButton::clicked, this, [this]() {
    syncFromWidgets();
    overrideActive_ = true;
    updateStatus();
    emit overrideInConfigurationRequested();
  });
  connect(ui_->buttonClearOverride, &QPushButton::clicked, this, [this]() {
    overrideActive_ = false;
    updateStatus();
    emit clearConfigurationOverrideRequested();
  });
  connect(ui_->buttonRestoreFactory, &QPushButton::clicked, this, &PreferencesDialog::restoreFactoryDefaultsRequested);
  connect(ui_->spinPlotTitleFontSize, SIGNAL(valueChanged(int)), this, SLOT(plotTitleSettingsChanged()));
  connect(ui_->comboPlotTitleWeight, SIGNAL(currentIndexChanged(int)), this, SLOT(plotTitleSettingsChanged()));
  connect(ui_->checkPlotTitleAutoColor, SIGNAL(stateChanged(int)), this, SLOT(plotTitleSettingsChanged()));
  connect(ui_->comboTheme, SIGNAL(currentIndexChanged(int)), this, SLOT(plotTitleSettingsChanged()));
  updateStatus();
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
  updatePlotTitlePreview();
}

QString PreferencesDialog::themeId() const {
  return themeId_;
}

void PreferencesDialog::setOpenGLCanvasEnabled(bool enabled) {
  openGLCanvasEnabled_ = enabled;
  ui_->checkOpenGLCanvas->setChecked(enabled);
}

bool PreferencesDialog::isOpenGLCanvasEnabled() const {
  return openGLCanvasEnabled_;
}

void PreferencesDialog::setPlotTitleStyle(const PlotTitleStyle& style) {
  plotTitleStyle_ = style;
  plotTitleStyle_.fontSize = PlotTitleStyle::clampFontSize(plotTitleStyle_.fontSize);
  const QSignalBlocker blockerSpin(ui_->spinPlotTitleFontSize);
  const QSignalBlocker blockerWeight(ui_->comboPlotTitleWeight);
  const QSignalBlocker blockerAuto(ui_->checkPlotTitleAutoColor);
  ui_->spinPlotTitleFontSize->setValue(plotTitleStyle_.fontSize);
  ui_->comboPlotTitleWeight->setCurrentIndex(plotTitleStyle_.bold ? 1 : 0);
  ui_->checkPlotTitleAutoColor->setChecked(plotTitleStyle_.autoColor);
  ui_->labelPlotTitleColorSwatch->setEnabled(!plotTitleStyle_.autoColor);
  updatePlotTitleColorSwatch();
  updatePlotTitlePreview();
}

PlotTitleStyle PreferencesDialog::plotTitleStyle() const {
  return plotTitleStyle_;
}

void PreferencesDialog::setOverrideActive(bool active) {
  overrideActive_ = active;
  updateStatus();
}

bool PreferencesDialog::isOverrideActive() const {
  return overrideActive_;
}

bool PreferencesDialog::eventFilter(QObject* object, QEvent* event) {
  if ((object == ui_->labelPlotTitleColorSwatch) && ui_->labelPlotTitleColorSwatch->isEnabled() &&
      (event->type() == QEvent::MouseButtonPress)) {
    QColorDialog dialog(this);
    Theme::apply(&dialog);
    dialog.setOptions(QColorDialog::DontUseNativeDialog);
    dialog.setCurrentColor(plotTitleStyle_.customColor);

    if (dialog.exec() == QDialog::Accepted) {
      plotTitleStyle_.customColor = dialog.currentColor();
      updatePlotTitleColorSwatch();
      updatePlotTitlePreview();
    }
  }

  return QDialog::eventFilter(object, event);
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

void PreferencesDialog::updateStatus() {
  ui_->labelStatus->setText(overrideActive_ ? QStringLiteral("This configuration overrides user defaults")
                                            : QStringLiteral("Using user defaults"));
  ui_->buttonClearOverride->setEnabled(overrideActive_);
}

PlotTitleStyle PreferencesDialog::selectedPlotTitleStyle() const {
  PlotTitleStyle style;
  style.fontSize = ui_->spinPlotTitleFontSize->value();
  style.bold = ui_->comboPlotTitleWeight->currentIndex() == 1;
  style.autoColor = ui_->checkPlotTitleAutoColor->isChecked();
  style.customColor = plotTitleStyle_.customColor;
  if (!style.customColor.isValid()) {
    style.customColor = PlotTitleStyle::factory().customColor;
  }
  return style;
}

void PreferencesDialog::updatePlotTitleColorSwatch() {
  const QColor color = plotTitleStyle_.customColor.isValid() ? plotTitleStyle_.customColor : PlotTitleStyle::factory().customColor;
  QPalette palette = ui_->labelPlotTitleColorSwatch->palette();
  palette.setColor(QPalette::Window, color);
  palette.setColor(QPalette::WindowText, (color.lightnessF() > 0.5) ? Qt::black : Qt::white);
  ui_->labelPlotTitleColorSwatch->setPalette(palette);
  ui_->labelPlotTitleColorSwatch->setText(color.name().toUpper());
}

void PreferencesDialog::updatePlotTitlePreview() {
  const PlotTitleStyle previewStyle = selectedPlotTitleStyle();
  const Theme::Id themeId = Theme::fromId(selectedThemeId());
  const QColor background = Theme::plotBackground(themeId);
  const QColor foreground = previewStyle.resolvedColor(themeId);

  QFont font = previewStyle.toFont(ui_->labelPlotTitlePreview->font());
  ui_->labelPlotTitlePreview->setFont(font);

  QPalette palette = ui_->labelPlotTitlePreview->palette();
  palette.setColor(QPalette::Window, background);
  palette.setColor(QPalette::WindowText, foreground);
  ui_->labelPlotTitlePreview->setAutoFillBackground(true);
  ui_->labelPlotTitlePreview->setPalette(palette);
}

void PreferencesDialog::syncFromWidgets() {
  timeZoneId_ = selectedTimeZoneId();
  themeId_ = selectedThemeId();
  openGLCanvasEnabled_ = ui_->checkOpenGLCanvas->isChecked();
  plotTitleStyle_ = selectedPlotTitleStyle();
}

void PreferencesDialog::plotTitleSettingsChanged() {
  plotTitleStyle_ = selectedPlotTitleStyle();
  ui_->labelPlotTitleColorSwatch->setEnabled(!plotTitleStyle_.autoColor);
  updatePlotTitleColorSwatch();
  updatePlotTitlePreview();
}

void PreferencesDialog::acceptDialog() {
  syncFromWidgets();
  accept();
}

}  // namespace rqt_multiplot
