/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TopicFieldTreeWidget.hpp"

#include <algorithm>
#include <tuple>

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHeaderView>
#include <QMimeData>
#include <QResizeEvent>
#include <QSet>
#include <QTreeView>
#include <QTreeWidgetItem>

#include "rqt_multiplot/MessageFieldAccess.hpp"

namespace rqt_multiplot {

namespace {

constexpr int kKeyRole = Qt::UserRole;
constexpr int kKindRole = Qt::UserRole + 1;
constexpr int kPathRole = Qt::UserRole + 2;
constexpr int kDefinitionRole = Qt::UserRole + 3;
constexpr int kTopicRole = Qt::UserRole + 4;
constexpr int kTypeRole = Qt::UserRole + 5;
constexpr int kDiagnosticStatusRole = Qt::UserRole + 6;
constexpr int kDiagnosticHardwareIdRole = Qt::UserRole + 7;
constexpr int kDiagnosticKeyRole = Qt::UserRole + 8;
constexpr QRgb kErrorColor = 0xc62828;

// Field names cannot start with '#', so these paths never collide with message field paths.
const QString kDiagnosticsPath = QStringLiteral("#diagnostics");

constexpr int kLeadingColumnShareNumerator = 2;
constexpr int kLeadingColumnShareDenominator = 3;

QString joinPath(const QString& prefix, const QString& name) {
  return prefix.isEmpty() ? name : prefix + "/" + name;
}

void collectExpandedPaths(const QTreeWidgetItem* item, QSet<QString>& paths) {
  for (int i = 0; i < item->childCount(); ++i) {
    const QTreeWidgetItem* child = item->child(i);
    if (child->isExpanded()) {
      paths.insert(child->data(0, kPathRole).toString());
    }
    collectExpandedPaths(child, paths);
  }
}

void restoreExpandedPaths(QTreeWidgetItem* item, const QSet<QString>& paths) {
  for (int i = 0; i < item->childCount(); ++i) {
    QTreeWidgetItem* child = item->child(i);
    const QString path = child->data(0, kPathRole).toString();
    if (!path.isEmpty() && paths.contains(path)) {
      child->setExpanded(true);
    }
    restoreExpandedPaths(child, paths);
  }
}

DiagnosticKeyRef diagnosticRefOf(const QTreeWidgetItem* item) {
  return {item->data(0, kDiagnosticStatusRole).toString(), item->data(0, kDiagnosticHardwareIdRole).toString(),
          item->data(0, kDiagnosticKeyRole).toString()};
}

bool diagnosticKeyLess(const DiagnosticKeyRef& lhs, const DiagnosticKeyRef& rhs) {
  return std::tie(lhs.status, lhs.hardwareId, lhs.key) < std::tie(rhs.status, rhs.hardwareId, rhs.key);
}

QVector<TopicFieldRef> withoutDuplicates(const QVector<TopicFieldRef>& refs) {
  QVector<TopicFieldRef> unique;
  QSet<QString> seen;
  for (const auto& ref : refs) {
    const QString id = QStringList({ref.topic, ref.type, ref.field, ref.diagnostic.status, ref.diagnostic.hardwareId, ref.diagnostic.key})
                           .join(QLatin1Char('\n'));
    if (!seen.contains(id)) {
      seen.insert(id);
      unique.append(ref);
    }
  }
  return unique;
}

class LeadingColumnGrowFilter : public QObject {
 public:
  explicit LeadingColumnGrowFilter(QTreeView* view) : QObject(view), view_(view) {}

  bool eventFilter(QObject* watched, QEvent* event) override {
    const auto* resizeEvent = (event->type() == QEvent::Resize) ? dynamic_cast<QResizeEvent*>(event) : nullptr;
    if (resizeEvent != nullptr) {
      QHeaderView* header = view_->header();
      const int newWidth = resizeEvent->size().width();
      const int oldWidth = resizeEvent->oldSize().width();
      const int leadingWidth = (oldWidth > 0) ? header->sectionSize(0) + newWidth - oldWidth
                                              : newWidth * kLeadingColumnShareNumerator / kLeadingColumnShareDenominator;
      header->resizeSection(0, std::max(header->minimumSectionSize(), leadingWidth));
    }
    return QObject::eventFilter(watched, event);
  }

 private:
  QTreeView* view_;
};

}  // namespace

void makeLeadingColumnGrow(QTreeView* view) {
  QHeaderView* header = view->header();
  header->setStretchLastSection(true);
  header->setSectionResizeMode(QHeaderView::Interactive);
  view->viewport()->installEventFilter(new LeadingColumnGrowFilter(view));
}

TopicFieldTreeWidget::TopicFieldTreeWidget(QWidget* parent) : QTreeWidget(parent) {
  qRegisterMetaType<MessageFieldType>();
  setObjectName(QStringLiteral("topicFieldTree"));
  setColumnCount(2);
  setIndentation(16);
  headerItem()->setText(0, tr("Name"));
  headerItem()->setText(1, tr("Type"));
  makeLeadingColumnGrow(this);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setDragEnabled(true);
  setDragDropMode(QAbstractItemView::DragOnly);
  setDefaultDropAction(Qt::CopyAction);
  setFrameShape(QFrame::NoFrame);
}

TopicFieldTreeWidget::~TopicFieldTreeWidget() = default;

void TopicFieldTreeWidget::addTopic(const QString& key, const QString& topic, const QString& type, bool fromBag) {
  if (hasTopic(key)) {
    return;
  }

  auto* item = new QTreeWidgetItem();
  item->setText(0, fromBag ? topic + tr(" [bag]") : topic);
  item->setText(1, shortTypeName(type));
  item->setToolTip(1, type);
  item->setData(0, kKeyRole, key);
  item->setData(0, kKindRole, Node);
  item->setData(0, kPathRole, QString());
  item->setData(0, kTopicRole, topic);
  item->setData(0, kTypeRole, type);
  setDraggable(item, false);
  addTopLevelItem(item);

  replaceChildrenWithStatus(item, tr("Loading..."), false);
  item->setExpanded(true);
}

void TopicFieldTreeWidget::removeTopic(const QString& key) {
  arrayLengths_.remove(key);
  diagnosticKeys_.remove(key);
  QTreeWidgetItem* item = topicItem(key);
  if (item != nullptr) {
    delete takeTopLevelItem(indexOfTopLevelItem(item));
  }
}

bool TopicFieldTreeWidget::hasTopic(const QString& key) const {
  return topicItem(key) != nullptr;
}

QStringList TopicFieldTreeWidget::topicKeys() const {
  QStringList keys;
  for (int i = 0; i < topLevelItemCount(); ++i) {
    keys.append(topLevelItem(i)->data(0, kKeyRole).toString());
  }
  return keys;
}

QTreeWidgetItem* TopicFieldTreeWidget::topicItem(const QString& key) const {
  for (int i = 0; i < topLevelItemCount(); ++i) {
    if (topLevelItem(i)->data(0, kKeyRole).toString() == key) {
      return topLevelItem(i);
    }
  }
  return nullptr;
}

void TopicFieldTreeWidget::setTopicDefinition(const QString& key, const MessageFieldType& definition) {
  QTreeWidgetItem* item = topicItem(key);
  if (item == nullptr) {
    return;
  }

  item->setData(0, kDefinitionRole, QVariant::fromValue(definition));
  setDraggable(item, !plottableLeaves(definition, QString()).isEmpty());
  buildTopicFields(item);
  item->setExpanded(true);
}

void TopicFieldTreeWidget::setTopicArrayLengths(const QString& key, const QHash<QString, int>& lengths) {
  arrayLengths_.insert(key, lengths);
  QTreeWidgetItem* item = topicItem(key);
  if ((item != nullptr) && item->data(0, kDefinitionRole).isValid()) {
    buildTopicFields(item);
  }
}

void TopicFieldTreeWidget::setTopicDiagnosticKeys(const QString& key, const QVector<DiagnosticKeyRef>& keys) {
  diagnosticKeys_.insert(key, keys);
  QTreeWidgetItem* item = topicItem(key);
  if ((item != nullptr) && item->data(0, kDefinitionRole).isValid()) {
    buildTopicFields(item);
  }
}

void TopicFieldTreeWidget::buildTopicFields(QTreeWidgetItem* root) {
  QSet<QString> expandedPaths;
  collectExpandedPaths(root, expandedPaths);
  qDeleteAll(root->takeChildren());

  const QString key = root->data(0, kKeyRole).toString();
  const auto definition = root->data(0, kDefinitionRole).value<MessageFieldType>();
  const auto lengthsIt = arrayLengths_.constFind(key);
  const QHash<QString, int>* lengths = (lengthsIt != arrayLengths_.constEnd()) ? &lengthsIt.value() : nullptr;
  for (const auto& member : definition.members) {
    addField(root, member.first, member.second, member.first, lengths);
  }
  if (isDiagnosticArrayTypeName(root->data(0, kTypeRole).toString().toStdString())) {
    const auto keysIt = diagnosticKeys_.constFind(key);
    addDiagnosticValues(root, (keysIt != diagnosticKeys_.constEnd()) ? &keysIt.value() : nullptr);
  }
  restoreExpandedPaths(root, expandedPaths);
}

void TopicFieldTreeWidget::addDiagnosticValues(QTreeWidgetItem* root, const QVector<DiagnosticKeyRef>* keys) {
  auto* group = new QTreeWidgetItem();
  root->insertChild(0, group);
  group->setText(0, tr("Diagnostic values"));
  group->setData(0, kKindRole, Unplottable);
  group->setData(0, kPathRole, kDiagnosticsPath);
  group->setFlags(Qt::ItemIsEnabled);

  if (keys == nullptr) {
    addStatusChild(group, tr("Waiting for a message..."), false);
    return;
  }
  if (keys->isEmpty()) {
    addStatusChild(group, tr("No diagnostic values"), false);
    return;
  }

  QVector<DiagnosticKeyRef> sorted = *keys;
  std::sort(sorted.begin(), sorted.end(), diagnosticKeyLess);
  QTreeWidgetItem* status = nullptr;
  for (const auto& ref : sorted) {
    if ((status == nullptr) || (status->data(0, kDiagnosticStatusRole).toString() != ref.status) ||
        (status->data(0, kDiagnosticHardwareIdRole).toString() != ref.hardwareId)) {
      status = new QTreeWidgetItem(group);
      const QString label = ref.hardwareId.isEmpty() ? ref.status : ref.status + QStringLiteral(" [") + ref.hardwareId + ']';
      status->setText(0, label);
      status->setToolTip(0, label);
      status->setData(0, kKindRole, DiagnosticStatus);
      status->setData(0, kPathRole, joinPath(kDiagnosticsPath, ref.status + '\n' + ref.hardwareId));
      status->setData(0, kDiagnosticStatusRole, ref.status);
      status->setData(0, kDiagnosticHardwareIdRole, ref.hardwareId);
      setDraggable(status, true);
    }
    auto* item = new QTreeWidgetItem(status);
    item->setText(0, ref.key);
    item->setToolTip(0, ref.key);
    item->setData(0, kKindRole, DiagnosticKey);
    item->setData(0, kDiagnosticStatusRole, ref.status);
    item->setData(0, kDiagnosticHardwareIdRole, ref.hardwareId);
    item->setData(0, kDiagnosticKeyRole, ref.key);
    setDraggable(item, true);
  }
}

// NOLINTNEXTLINE(readability-make-member-function-const)
void TopicFieldTreeWidget::setTopicError(const QString& key, const QString& error) {
  QTreeWidgetItem* item = topicItem(key);
  if (item != nullptr) {
    replaceChildrenWithStatus(item, error, true);
  }
}

QVector<TopicFieldRef> TopicFieldTreeWidget::refsForItems(const QList<QTreeWidgetItem*>& items) {
  return collectRefs(items, false);
}

QVector<TopicFieldRef> TopicFieldTreeWidget::expandedRefsForItems(const QList<QTreeWidgetItem*>& items) {
  return collectRefs(items, true);
}

QVector<TopicFieldRef> TopicFieldTreeWidget::collectRefs(const QList<QTreeWidgetItem*>& items, bool expandArrays) {
  QVector<TopicFieldRef> refs;
  for (QTreeWidgetItem* item : items) {
    appendItemRefs(item, expandArrays, refs);
  }
  return withoutDuplicates(refs);
}

void TopicFieldTreeWidget::appendItemRefs(QTreeWidgetItem* item, bool expandArrays, QVector<TopicFieldRef>& refs) {
  const QTreeWidgetItem* root = rootOf(item);
  const QString topic = root->data(0, kTopicRole).toString();
  const QString type = root->data(0, kTypeRole).toString();
  const QString path = item->data(0, kPathRole).toString();

  switch (item->data(0, kKindRole).toInt()) {
    case Leaf:
      refs.append({topic, type, path});
      break;
    case Array: {
      const int before = static_cast<int>(refs.count());
      if (expandArrays) {
        for (int i = 0; i < item->childCount(); ++i) {
          appendElementRefs(item->child(i), topic, type, refs);
        }
      }
      if (refs.count() == before) {
        for (const QString& field : arrayWildcardFields(item->data(0, kDefinitionRole).value<MessageFieldType>(), path)) {
          refs.append({topic, type, field});
        }
      }
      break;
    }
    case Node:
      for (const QString& leaf : plottableLeaves(item->data(0, kDefinitionRole).value<MessageFieldType>(), path)) {
        refs.append({topic, type, leaf});
      }
      break;
    case DiagnosticStatus:
      for (int i = 0; i < item->childCount(); ++i) {
        refs.append({topic, type, QString(), diagnosticRefOf(item->child(i))});
      }
      break;
    case DiagnosticKey:
      refs.append({topic, type, QString(), diagnosticRefOf(item)});
      break;
    default:
      break;
  }
}

void TopicFieldTreeWidget::appendElementRefs(const QTreeWidgetItem* element, const QString& topic, const QString& type,
                                             QVector<TopicFieldRef>& refs) {
  const QString path = element->data(0, kPathRole).toString();
  switch (element->data(0, kKindRole).toInt()) {
    case Leaf:
      refs.append({topic, type, path});
      break;
    case Node:
      for (int i = 0; i < element->childCount(); ++i) {
        appendElementRefs(element->child(i), topic, type, refs);
      }
      break;
    case Array:
      for (const QString& field : arrayWildcardFields(element->data(0, kDefinitionRole).value<MessageFieldType>(), path)) {
        refs.append({topic, type, field});
      }
      break;
    default:
      break;
  }
}

QStringList TopicFieldTreeWidget::mimeTypes() const {
  return {kTopicFieldsMimeType};
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
QMimeData* TopicFieldTreeWidget::mimeData(const QList<QTreeWidgetItem*>& items) const {
#else
QMimeData* TopicFieldTreeWidget::mimeData(const QList<QTreeWidgetItem*> items) const {
#endif
  const QVector<TopicFieldRef> refs = refsForItems(items);
  if (refs.isEmpty()) {
    return nullptr;
  }
  auto* data = new QMimeData();
  data->setData(kTopicFieldsMimeType, encodeTopicFields(refs));
  const QVector<TopicFieldRef> expanded = expandedRefsForItems(items);
  if (expanded != refs) {
    data->setData(kTopicFieldsExpandedMimeType, encodeTopicFields(expanded));
  }
  return data;
}

void TopicFieldTreeWidget::addField(QTreeWidgetItem* parent, const QString& name, const MessageFieldType& fieldType, const QString& path,
                                    const QHash<QString, int>* lengths) {
  auto* item = new QTreeWidgetItem(parent);
  item->setText(0, name);
  item->setText(1, fieldType.identifier);
  item->setData(0, kPathRole, path);

  QFont typeFont = item->font(1);
  typeFont.setItalic(true);
  item->setFont(1, typeFont);

  if (fieldType.isNumeric) {
    item->setData(0, kKindRole, Leaf);
    setDraggable(item, true);
  } else if (fieldType.isMessage()) {
    item->setData(0, kKindRole, Node);
    item->setData(0, kDefinitionRole, QVariant::fromValue(fieldType));
    setDraggable(item, !plottableLeaves(fieldType, path).isEmpty());
    for (const auto& member : fieldType.members) {
      addField(item, member.first, member.second, joinPath(path, member.first), lengths);
    }
  } else if (fieldType.isNumericArray() || (fieldType.isArray() && fieldType.elementType && !fieldType.elementType->isBuiltin())) {
    item->setData(0, kKindRole, Array);
    item->setData(0, kDefinitionRole, QVariant::fromValue(fieldType));
    setDraggable(item, !arrayWildcardFields(fieldType, path).isEmpty());
    addArrayElements(item, name, fieldType, path, lengths);
  } else {
    item->setData(0, kKindRole, Unplottable);
    item->setFlags(Qt::NoItemFlags);
  }
}

void TopicFieldTreeWidget::addArrayElements(QTreeWidgetItem* item, const QString& name, const MessageFieldType& arrayType,
                                            const QString& path, const QHash<QString, int>* lengths) {
  int count = static_cast<int>(arrayType.arraySize);
  if (arrayType.isDynamicArray) {
    if (lengths == nullptr) {
      addStatusChild(item, tr("Waiting for a message..."), false);
      return;
    }
    count = lengths->value(path, 0);
  }

  if (count <= 0) {
    addStatusChild(item, tr("Empty"), false);
    return;
  }

  const int shown = std::min(count, kMaxArrayElementsShown);
  for (int i = 0; i < shown; ++i) {
    addField(item, QStringLiteral("%1[%2]").arg(name).arg(i), *arrayType.elementType, joinPath(path, QString::number(i)), lengths);
  }
  if (count > shown) {
    addStatusChild(item, tr("%1 more elements not shown").arg(count - shown), false);
  }
}

void TopicFieldTreeWidget::replaceChildrenWithStatus(QTreeWidgetItem* item, const QString& text, bool isError) {
  qDeleteAll(item->takeChildren());
  addStatusChild(item, text, isError);
}

void TopicFieldTreeWidget::addStatusChild(QTreeWidgetItem* parent, const QString& text, bool isError) {
  auto* status = new QTreeWidgetItem(parent);
  status->setText(0, text);
  status->setToolTip(0, text);
  status->setData(0, kKindRole, Unplottable);
  status->setFlags(Qt::ItemIsEnabled);
  QFont font = status->font(0);
  font.setItalic(true);
  status->setFont(0, font);
  if (isError) {
    status->setForeground(0, QBrush(QColor(kErrorColor)));
  }
}

QTreeWidgetItem* TopicFieldTreeWidget::rootOf(QTreeWidgetItem* item) {
  while (item->parent() != nullptr) {
    item = item->parent();
  }
  return item;
}

void TopicFieldTreeWidget::setDraggable(QTreeWidgetItem* item, bool draggable) {
  item->setFlags(draggable ? (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled) : Qt::ItemIsEnabled);
}

}  // namespace rqt_multiplot
