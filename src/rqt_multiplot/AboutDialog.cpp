/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/AboutDialog.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "rqt_multiplot/Theme.hpp"

namespace rqt_multiplot {

namespace {

#ifndef RQT_MULTIPLOT_VERSION
#define RQT_MULTIPLOT_VERSION "unknown"
#endif

constexpr auto kDescription =
    "rqt_multiplot provides a GUI plugin for visualizing numeric values in multiple 2D plots using the Qwt plotting backend.";
constexpr auto kProjectUrl = "https://github.com/samuelba/rqt_multiplot_plugin";
constexpr auto kLgplUrl = "https://www.gnu.org/licenses/lgpl-3.0.html";
constexpr auto kQwtUrl = "https://qwt.sourceforge.io";
constexpr auto kQwtLicenseUrl = "https://qwt.sourceforge.io/qwtlicense.html";

QString styledLink(const QString& href, const QString& label, const QString& linkColor) {
  return QStringLiteral("<a href=\"%1\" style=\"color:%2; text-decoration:underline;\">%3</a>").arg(href, linkColor, label);
}

QString buildBodyText(const QString& textColor, const QString& linkColor) {
  return QStringLiteral(
             "<div style=\"color:%1;\">"
             "<h2>Multiplot</h2>"
             "<p>Version %2</p>"
             "<p>%3</p>"
             "<p>%4</p>"
             "<p>Copyright &copy; 2015 Ralf Kaestner</p>"
             "<p>Licensed under the %5.</p>"
             "<p>This program uses %6, licensed under the %7.</p>"
             "</div>")
      .arg(textColor, QString::fromLatin1(RQT_MULTIPLOT_VERSION), QString::fromUtf8(kDescription),
           styledLink(QString::fromLatin1(kProjectUrl), QString::fromLatin1(kProjectUrl), linkColor),
           styledLink(QString::fromLatin1(kLgplUrl), QStringLiteral("GNU Lesser General Public License (LGPL) v3.0"), linkColor),
           styledLink(QString::fromLatin1(kQwtUrl), QStringLiteral("Qwt"), linkColor),
           styledLink(QString::fromLatin1(kQwtLicenseUrl), QStringLiteral("Qwt License, Version 1.0"), linkColor));
}

QString linkColorForTheme(Theme::Id themeId, const QPalette& themePalette) {
  if (themeId == Theme::Id::Dark) {
    return QStringLiteral("#7eb8ff");
  }
  return themePalette.color(QPalette::Link).name(QColor::HexRgb);
}

}  // namespace

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(tr("About Multiplot"));

  auto* layout = new QVBoxLayout(this);

  auto* label = new QLabel(this);
  label->setObjectName(QStringLiteral("aboutBodyLabel"));
  label->setTextFormat(Qt::RichText);
  label->setTextInteractionFlags(Qt::TextBrowserInteraction);
  label->setOpenExternalLinks(true);
  label->setWordWrap(true);
  layout->addWidget(label);

  auto* closeButton = new QPushButton(tr("Close"), this);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
  layout->addWidget(closeButton, 0, Qt::AlignRight);

  Theme::apply(this);
  const Theme::Id themeId = Theme::currentId();
  const QPalette themePalette = Theme::palette(themeId);
  const QString textColor = themePalette.color(QPalette::WindowText).name(QColor::HexRgb);
  const QString linkColor = linkColorForTheme(themeId, themePalette);
  bodyText_ = buildBodyText(textColor, linkColor);
  label->setText(bodyText_);
}

QString AboutDialog::bodyText() const {
  return bodyText_;
}

}  // namespace rqt_multiplot
