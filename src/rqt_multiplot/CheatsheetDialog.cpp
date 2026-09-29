/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/CheatsheetDialog.hpp"

#include <algorithm>
#include <vector>

#include <QCoreApplication>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "rqt_multiplot/Theme.hpp"

namespace rqt_multiplot {

namespace {

struct Row {
  QString input;
  QString action;
};

struct Section {
  QString objectName;
  QString title;
  QString tableObjectName;
  bool alignInputRight = true;
  std::vector<Row> rows;
};

QString dialogText(const char* source) {
  return QCoreApplication::translate("rqt_multiplot::CheatsheetDialog", source);
}

QKeySequence keySequence(int modifiers, int key) {
  return QKeySequence(static_cast<int>(modifiers) | key);
}

QString keyText(const QKeySequence& sequence) {
  return sequence.toString(QKeySequence::NativeText);
}

QTableWidget* createTable(QWidget* parent, const QString& objectName) {
  auto* table = new QTableWidget(parent);
  table->setObjectName(objectName);
  table->setColumnCount(2);
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setFocusPolicy(Qt::NoFocus);
  table->setShowGrid(false);
  table->setAlternatingRowColors(true);
  table->setWordWrap(false);
  table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  table->verticalHeader()->setVisible(false);
  table->horizontalHeader()->setVisible(false);
  table->horizontalHeader()->setStretchLastSection(true);
  table->horizontalHeader()->setHighlightSections(false);
  table->horizontalHeader()->setSectionsClickable(false);
  table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  return table;
}

void addRow(QTableWidget* table, const QString& input, const QString& action, bool alignInputRight) {
  if (input.trimmed().isEmpty()) {
    return;
  }

  const int row = table->rowCount();
  table->insertRow(row);

  auto* inputItem = new QTableWidgetItem(input);
  inputItem->setFlags(inputItem->flags() & ~Qt::ItemIsEditable);
  QFont inputFont = inputItem->font();
  inputFont.setBold(true);
  inputItem->setFont(inputFont);
  const Qt::Alignment inputAlignment = (alignInputRight ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  inputItem->setTextAlignment(inputAlignment);
#else
  inputItem->setTextAlignment(static_cast<int>(inputAlignment));
#endif
  table->setItem(row, 0, inputItem);

  auto* actionItem = new QTableWidgetItem(action);
  actionItem->setFlags(actionItem->flags() & ~Qt::ItemIsEditable);
  table->setItem(row, 1, actionItem);
}

void fitTableHeight(QTableWidget* table) {
  table->resizeRowsToContents();
  int height = (table->frameWidth() * 2) + 2;
  if (!table->horizontalHeader()->isHidden()) {
    height += table->horizontalHeader()->sizeHint().height();
  }
  for (int row = 0; row < table->rowCount(); ++row) {
    height += table->rowHeight(row);
  }
  table->setFixedHeight(height);
}

void alignInputColumn(const std::vector<QTableWidget*>& tables) {
  int width = 0;
  for (QTableWidget* table : tables) {
    table->resizeColumnToContents(0);
    width = std::max(width, table->columnWidth(0));
  }
  for (QTableWidget* table : tables) {
    table->setColumnWidth(0, width + 12);
  }
}

QWidget* buildSection(QWidget* parent, const Section& section) {
  const bool hasShortcut =
      std::any_of(section.rows.cbegin(), section.rows.cend(), [](const Row& row) { return !row.input.trimmed().isEmpty(); });
  if (!hasShortcut) {
    return nullptr;
  }

  auto* group = new QGroupBox(section.title, parent);
  group->setObjectName(section.objectName);

  auto* table = createTable(group, section.tableObjectName);
  table->setHorizontalHeaderLabels({dialogText(section.alignInputRight ? "Shortcut" : "Input"), dialogText("Action")});
  for (const Row& row : section.rows) {
    addRow(table, row.input, row.action, section.alignInputRight);
  }

  auto* layout = new QVBoxLayout(group);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->addWidget(table);
  return group;
}

QWidget* buildColumn(QWidget* parent, const QString& objectName, const std::vector<Section>& sections) {
  auto* column = new QWidget(parent);
  column->setObjectName(objectName);
  auto* layout = new QVBoxLayout(column);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(12);

  for (const Section& section : sections) {
    if (QWidget* group = buildSection(column, section)) {
      layout->addWidget(group);
    }
  }
  layout->addStretch();
  return column;
}

Row shortcutRow(const QKeySequence& sequence, const QString& action) {
  return Row{keyText(sequence), action};
}

std::vector<Section> leftSections() {
  return {
      {QStringLiteral("cheatsheetSectionFile"),
       dialogText("File"),
       QStringLiteral("cheatsheetFileTable"),
       true,
       {
           shortcutRow(QKeySequence::New, dialogText("New configuration")),
           shortcutRow(QKeySequence::Open, dialogText("Open configuration...")),
           shortcutRow(QKeySequence::Save, dialogText("Save configuration")),
           shortcutRow(QKeySequence::SaveAs, dialogText("Save configuration as...")),
           shortcutRow(QKeySequence::Quit, dialogText("Quit (standalone window only)")),
       }},
      {QStringLiteral("cheatsheetSectionImport"),
       dialogText("Import and export"),
       QStringLiteral("cheatsheetImportTable"),
       true,
       {
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_I), dialogText("Import from bag files...")),
           shortcutRow(keySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT), Qt::Key_I), dialogText("Add bag files...")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_E), dialogText("Export to image file...")),
       }},
      {QStringLiteral("cheatsheetSectionPlots"),
       dialogText("Plots"),
       QStringLiteral("cheatsheetPlotsTable"),
       true,
       {
           shortcutRow(QKeySequence(Qt::Key_Space), dialogText("Play / pause all plots")),
           shortcutRow(keySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT), Qt::Key_Delete),
                       dialogText("Clear all plots")),
           shortcutRow(QKeySequence(Qt::Key_Home), dialogText("Reset zoom of all plots in the active tab")),
       }},
      {QStringLiteral("cheatsheetSectionGeneral"),
       dialogText("General"),
       QStringLiteral("cheatsheetGeneralTable"),
       true,
       {
           shortcutRow(QKeySequence::Preferences, dialogText("Preferences...")),
           shortcutRow(QKeySequence::HelpContents, dialogText("Open this dialog")),
       }},
  };
}

std::vector<Section> rightSections() {
  return {
      {QStringLiteral("cheatsheetSectionTabs"),
       dialogText("Tabs"),
       QStringLiteral("cheatsheetTabsTable"),
       true,
       {
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_T), dialogText("New tab")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_W), dialogText("Close tab")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_PageDown), dialogText("Next tab")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_PageUp), dialogText("Previous tab")),
           {keyText(keySequence(Qt::ALT, Qt::Key_1)) + QStringLiteral(" ... ") + keyText(keySequence(Qt::ALT, Qt::Key_9)),
            dialogText("Select tab 1-9")},
       }},
      {QStringLiteral("cheatsheetSectionView"),
       dialogText("View"),
       QStringLiteral("cheatsheetViewTable"),
       true,
       {
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_B), dialogText("Show or hide the topic browser")),
           shortcutRow(keySequence(static_cast<int>(Qt::CTRL) | static_cast<int>(Qt::SHIFT), Qt::Key_B),
                       dialogText("Show or hide curve values")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_G), dialogText("Show or hide the grid")),
       }},
      {QStringLiteral("cheatsheetSectionCurves"),
       dialogText("Curves"),
       QStringLiteral("cheatsheetCurvesTable"),
       true,
       {
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_A), dialogText("Select all curves (plot config)")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_C), dialogText("Copy selected curves (plot config)")),
           shortcutRow(keySequence(Qt::CTRL, Qt::Key_V), dialogText("Paste curves (plot config)")),
           shortcutRow(QKeySequence(Qt::Key_Delete), dialogText("Remove selected curves (plot config)")),
       }},
  };
}

Section mouseSection() {
  return {
      QStringLiteral("cheatsheetSectionMouse"),
      dialogText("Mouse"),
      QStringLiteral("mouseActionsTable"),
      false,
      {
          {dialogText("Left drag"), dialogText("Pan")},
          {dialogText("Ctrl + left drag"), dialogText("Draw a rectangle to zoom")},
          {dialogText("Mouse wheel"), dialogText("Zoom in / out")},
          {dialogText("Right drag"), dialogText("Zoom. Drag right or down to zoom in on that axis, left or up to zoom out")},
          {dialogText("Right click (no drag)"), dialogText("Open plot context menu")},
          {dialogText("Click a legend item"), dialogText("Toggle that curve's visibility")},
          {dialogText("Drag a legend item onto another plot"), dialogText("Copy that curve")},
      },
  };
}

void finishColumn(QWidget* column) {
  const QList<QTableWidget*> found = column->findChildren<QTableWidget*>();
  for (QTableWidget* table : found) {
    table->ensurePolished();
  }
  alignInputColumn(std::vector<QTableWidget*>(found.cbegin(), found.cend()));
  for (QTableWidget* table : found) {
    fitTableHeight(table);
  }
}

}  // namespace

CheatsheetDialog::CheatsheetDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(dialogText("Keyboard shortcuts"));

  auto* left = buildColumn(this, QStringLiteral("cheatsheetLeftColumn"), leftSections());
  auto* right = buildColumn(this, QStringLiteral("cheatsheetRightColumn"), rightSections());

  auto* columns = new QHBoxLayout();
  columns->setObjectName(QStringLiteral("cheatsheetColumns"));
  columns->setSpacing(16);
  columns->addWidget(left, 1);
  columns->addWidget(right, 1);

  auto* closeButton = new QPushButton(dialogText("Close"), this);
  closeButton->setAutoDefault(true);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

  auto* content = new QWidget(this);
  auto* contentLayout = new QVBoxLayout(content);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->addLayout(columns);
  if (QWidget* mouse = buildSection(content, mouseSection())) {
    contentLayout->addWidget(mouse);
  }

  auto* scroll = new QScrollArea(this);
  scroll->setObjectName(QStringLiteral("cheatsheetScroll"));
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidget(content);

  auto* layout = new QVBoxLayout(this);
  layout->addWidget(scroll, 1);
  layout->addWidget(closeButton, 0, Qt::AlignRight);

  Theme::apply(this);
  finishColumn(left);
  finishColumn(right);
  if (auto* mouseTable = findChild<QTableWidget*>(QStringLiteral("mouseActionsTable"))) {
    mouseTable->ensurePolished();
    mouseTable->resizeColumnsToContents();
    fitTableHeight(mouseTable);
  }

  content->adjustSize();
  const int fittedHeight = content->sizeHint().height() + closeButton->sizeHint().height() + layout->spacing() +
                           layout->contentsMargins().top() + layout->contentsMargins().bottom();
  resize(1040, std::max(fittedHeight, 640));
}

}  // namespace rqt_multiplot
