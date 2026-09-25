/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/Theme.hpp"

#include <QAbstractButton>
#include <QAbstractItemView>
#include <QAction>
#include <QChildEvent>
#include <QComboBox>
#include <QEvent>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QProxyStyle>
#include <QSize>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QVariant>

#include "rqt_multiplot/PackageResource.hpp"

namespace rqt_multiplot {

namespace {

constexpr auto kThemeFilterProperty = "rqt_multiplot_theme_filter";

bool isTopicBrowserList(const QWidget* widget) {
  for (const QWidget* current = widget; current != nullptr; current = current->parentWidget()) {
    if (current->objectName() == QLatin1String("topicBrowserList")) {
      return true;
    }
  }
  return false;
}

// Fusion builds the checkbox outline from window.darker(), which sits on the dark list background.
void drawVisibleCheckBorder(const QStyleOption* option, QPainter* painter) {
  const bool keyboardFocus = ((option->state & QStyle::State_HasFocus) != 0) && ((option->state & QStyle::State_KeyboardFocusChange) != 0);
  if (keyboardFocus) {
    return;
  }

  const QPalette::ColorGroup group = ((option->state & QStyle::State_Enabled) != 0) ? QPalette::Normal : QPalette::Disabled;
  const QColor border = option->palette.color(group, QPalette::Text);
  const QRect rect = option->rect.adjusted(0, 0, -1, -1);
  painter->setPen(Qt::NoPen);
  painter->fillRect(rect.left(), rect.top(), rect.width(), 1, border);
  painter->fillRect(rect.left(), rect.bottom(), rect.width(), 1, border);
  painter->fillRect(rect.left(), rect.top(), 1, rect.height(), border);
  painter->fillRect(rect.right(), rect.top(), 1, rect.height(), border);
}

class FusionThemeStyle : public QProxyStyle {
 public:
  FusionThemeStyle() : QProxyStyle(QStringLiteral("fusion")) { setObjectName(QStringLiteral("Fusion")); }

  void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override {
    QProxyStyle::drawPrimitive(element, option, painter, widget);
    const bool isItemCheck = (element == PE_IndicatorItemViewItemCheck) && (option != nullptr) && (painter != nullptr);
    if (!isItemCheck || (Theme::currentId() != Theme::Id::Dark) || !isTopicBrowserList(widget)) {
      return;
    }
    painter->save();
    drawVisibleCheckBorder(option, painter);
    painter->restore();
  }
};

QStyle* fusionStyle() {
  static QStyle* style = new FusionThemeStyle();
  return style;
}

class FusionMenuBarStyle : public QProxyStyle {
 public:
  FusionMenuBarStyle() : QProxyStyle(QStringLiteral("fusion")) { setObjectName(QStringLiteral("Fusion")); }

  int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const override {
    if (metric == PM_MenuBarHMargin || metric == PM_MenuBarPanelWidth || metric == PM_MenuBarItemSpacing) {
      return 0;
    }
    return QProxyStyle::pixelMetric(metric, option, widget);
  }

  void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const override {
    if (element == CE_MenuBarItem) {
      if (const auto* menuItem = qstyleoption_cast<const QStyleOptionMenuItem*>(option)) {
        QStyleOptionMenuItem background(*menuItem);
        background.text.clear();
        QProxyStyle::drawControl(element, &background, painter, widget);
        proxy()->drawItemText(painter, menuItem->rect,
                              Qt::AlignCenter | Qt::AlignVCenter | Qt::TextShowMnemonic | Qt::TextDontClip | Qt::TextSingleLine,
                              menuItem->palette, (menuItem->state & State_Enabled) != 0, menuItem->text, QPalette::ButtonText);
        return;
      }
    }
    QProxyStyle::drawControl(element, option, painter, widget);
  }
};

QStyle* fusionMenuBarStyle() {
  static QStyle* style = new FusionMenuBarStyle();
  return style;
}

void setFusionStyle(QWidget* widget) {
  QStyle* style = qobject_cast<QMenuBar*>(widget) != nullptr ? fusionMenuBarStyle() : fusionStyle();
  if (style != nullptr) {
    widget->setStyle(style);
  }
}

bool needsExplicitPalette(const QWidget* widget) {
  return (qobject_cast<const QMenu*>(widget) != nullptr) || (qobject_cast<const QMenuBar*>(widget) != nullptr) ||
         (qobject_cast<const QComboBox*>(widget) != nullptr) || (qobject_cast<const QAbstractItemView*>(widget) != nullptr);
}

void applyToWidget(QWidget* widget, const QPalette& pal);

class ThemeFilter : public QObject {
 public:
  ThemeFilter() : QObject(nullptr) {}

  bool eventFilter(QObject* watched, QEvent* event) override {
    if (event->type() == QEvent::ChildAdded) {
      auto* childEvent = dynamic_cast<QChildEvent*>(event);
      if (childEvent == nullptr) {
        return QObject::eventFilter(watched, event);
      }
      if (auto* widget = qobject_cast<QWidget*>(childEvent->child())) {
        applyToWidget(widget, Theme::palette(Theme::currentId()));
      }
    } else if (event->type() == QEvent::Show) {
      if (auto* menu = qobject_cast<QMenu*>(watched)) {
        applyToWidget(menu, Theme::palette(Theme::currentId()));
      }
    }
    return QObject::eventFilter(watched, event);
  }
};

ThemeFilter* themeFilter() {
  static ThemeFilter filter;
  return &filter;
}

void ensureFilter(QWidget* widget) {
  if (widget->property(kThemeFilterProperty).toBool()) {
    return;
  }
  widget->setProperty(kThemeFilterProperty, true);
  widget->installEventFilter(themeFilter());
}

void applyToWidget(QWidget* widget, const QPalette& pal) {
  setFusionStyle(widget);
  if (needsExplicitPalette(widget)) {
    widget->setPalette(pal);
    widget->setAutoFillBackground(true);
  }
  if (auto* combo = qobject_cast<QComboBox*>(widget)) {
    if (QAbstractItemView* view = combo->view()) {
      setFusionStyle(view);
      view->setPalette(pal);
    }
  }
  ensureFilter(widget);
}

}  // namespace

Theme::Id Theme::currentId_ = Theme::Id::Light;

Theme::Id Theme::currentId() {
  return currentId_;
}

Theme::Id Theme::fromId(const QString& id) {
  if (id.trimmed().toLower() == QLatin1String(kDarkId)) {
    return Id::Dark;
  }
  return Id::Light;
}

QString Theme::toId(Id id) {
  return (id == Id::Dark) ? QString::fromLatin1(kDarkId) : QString::fromLatin1(kLightId);
}

QPalette Theme::palette(Id id) {
  QPalette palette;
  if (id == Id::Dark) {
    const QColor window(0x35, 0x35, 0x35);
    const QColor base(0x25, 0x25, 0x25);
    const QColor text(0xe6, 0xe6, 0xe6);
    const QColor disabled(0x7f, 0x7f, 0x7f);
    const QColor highlight(0x2a, 0x82, 0xda);
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::ToolTipBase, window);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, highlight);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, Qt::black);
    palette.setColor(QPalette::Light, window.lighter(120));
    palette.setColor(QPalette::Mid, QColor(0x60, 0x60, 0x60));
    palette.setColor(QPalette::Dark, QColor(0x1a, 0x1a, 0x1a));
    palette.setColor(QPalette::Shadow, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    return palette;
  }

  const QColor disabled(0x7f, 0x7f, 0x7f);
  palette.setColor(QPalette::Window, QColor(0xf0, 0xf0, 0xf0));
  palette.setColor(QPalette::WindowText, Qt::black);
  palette.setColor(QPalette::Base, Qt::white);
  palette.setColor(QPalette::AlternateBase, QColor(0xe8, 0xe8, 0xe8));
  palette.setColor(QPalette::ToolTipBase, Qt::white);
  palette.setColor(QPalette::ToolTipText, Qt::black);
  palette.setColor(QPalette::Text, Qt::black);
  palette.setColor(QPalette::Button, QColor(0xf0, 0xf0, 0xf0));
  palette.setColor(QPalette::ButtonText, Qt::black);
  palette.setColor(QPalette::BrightText, Qt::red);
  palette.setColor(QPalette::Link, QColor(0x2a, 0x82, 0xda));
  palette.setColor(QPalette::Highlight, QColor(0x2a, 0x82, 0xda));
  palette.setColor(QPalette::HighlightedText, Qt::white);
  palette.setColor(QPalette::Mid, QColor(0xa0, 0xa0, 0xa0));
  palette.setColor(QPalette::Light, Qt::white);
  palette.setColor(QPalette::Dark, QColor(0x80, 0x80, 0x80));
  palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
  palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
  return palette;
}

QColor Theme::plotBackground(Id id) {
  return (id == Id::Dark) ? QColor(0x1e, 0x1e, 0x1e) : QColor(Qt::white);
}

QColor Theme::plotForeground(Id id) {
  return (id == Id::Dark) ? QColor(0xe6, 0xe6, 0xe6) : QColor(Qt::black);
}

QColor Theme::iconColor() {
  return palette(currentId_).color(QPalette::WindowText);
}

QColor Theme::disabledIconColor() {
  return (currentId_ == Id::Dark) ? QColor(0x75, 0x75, 0x75) : QColor(0x9e, 0x9e, 0x9e);
}

void Theme::apply(QWidget* root) {
  apply(root, currentId_);
}

void Theme::apply(QWidget* root, Id id) {
  currentId_ = id;
  if (root == nullptr) {
    return;
  }

  const QPalette pal = palette(id);
  root->setPalette(pal);
  root->setAutoFillBackground(true);
  applyToWidget(root, pal);

  const QList<QWidget*> widgets = root->findChildren<QWidget*>();
  for (QWidget* widget : widgets) {
    applyToWidget(widget, pal);
  }
  refreshIcons(root);
}

void Theme::refreshIcons(QWidget* root) {
  if (root == nullptr) {
    return;
  }

  const auto refreshButton = [](QAbstractButton* button) {
    const QVariant path = button->property(kThemeIconPathProperty);
    if (!path.isValid()) {
      return;
    }
    QSize size = button->property(kThemeIconSizeProperty).toSize();
    if (!size.isValid()) {
      size = QSize(16, 16);
    }
    button->setIcon(packageIcon(path.toString(), size));
  };

  if (auto* button = qobject_cast<QAbstractButton*>(root)) {
    refreshButton(button);
  }
  const QList<QAbstractButton*> buttons = root->findChildren<QAbstractButton*>();
  for (QAbstractButton* button : buttons) {
    refreshButton(button);
  }

  const auto refreshAction = [](QAction* action) {
    const QVariant path = action->property(kThemeIconPathProperty);
    if (!path.isValid()) {
      return;
    }
    QSize size = action->property(kThemeIconSizeProperty).toSize();
    if (!size.isValid()) {
      size = QSize(16, 16);
    }
    action->setIcon(packageIcon(path.toString(), size));
  };

  const QList<QAction*> actions = root->findChildren<QAction*>();
  for (QAction* action : actions) {
    refreshAction(action);
  }
}

}  // namespace rqt_multiplot
