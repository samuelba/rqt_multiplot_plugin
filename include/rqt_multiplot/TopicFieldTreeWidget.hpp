/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QTreeWidget>
#include <QVector>

#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"

class QMimeData;
class QTreeView;
class QTreeWidgetItem;

namespace rqt_multiplot {

inline QString shortTypeName(const QString& type) {
  return type.section('/', -1);
}

// Column 0 stays user-resizable and takes all width changes of the view; the last column fills the rest.
void makeLeadingColumnGrow(QTreeView* view);

class TopicFieldTreeWidget : public QTreeWidget {
  Q_OBJECT
 public:
  explicit TopicFieldTreeWidget(QWidget* parent = nullptr);
  ~TopicFieldTreeWidget() override;

  void addTopic(const QString& key, const QString& topic, const QString& type, bool fromBag);
  void removeTopic(const QString& key);
  bool hasTopic(const QString& key) const;
  QStringList topicKeys() const;
  QTreeWidgetItem* topicItem(const QString& key) const;
  void setTopicDefinition(const QString& key, const MessageFieldType& definition);
  void setTopicArrayLengths(const QString& key, const QHash<QString, int>& lengths);
  void setTopicDiagnosticKeys(const QString& key, const QVector<DiagnosticKeyRef>& keys);
  void setTopicError(const QString& key, const QString& error);

  static QVector<TopicFieldRef> refsForItems(const QList<QTreeWidgetItem*>& items);
  // Like refsForItems, but arrays give one ref per listed element instead of one wildcard ref per leaf.
  static QVector<TopicFieldRef> expandedRefsForItems(const QList<QTreeWidgetItem*>& items);

 protected:
  QStringList mimeTypes() const override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;
#else
  QMimeData* mimeData(const QList<QTreeWidgetItem*> items) const override;
#endif

 private:
  enum ItemKind { Unplottable, Leaf, Array, Node, DiagnosticStatus, DiagnosticKey, TopicMetricGroup, TopicMetricLeaf };

  QHash<QString, QHash<QString, int>> arrayLengths_;
  QHash<QString, QVector<DiagnosticKeyRef>> diagnosticKeys_;

  void buildTopicFields(QTreeWidgetItem* root);
  static void addDiagnosticValues(QTreeWidgetItem* root, const QVector<DiagnosticKeyRef>* keys);
  static void addTopicMetrics(QTreeWidgetItem* root, bool withDelay);
  static QVector<TopicFieldRef> rootFieldRefsOf(const QTreeWidgetItem* root);
  static QVector<TopicFieldRef> topicMetricRefsOf(const QTreeWidgetItem* root);
  static QVector<TopicFieldRef> collectRefs(const QList<QTreeWidgetItem*>& items, bool expandArrays);
  static void appendItemRefs(QTreeWidgetItem* item, bool expandArrays, QVector<TopicFieldRef>& refs);
  static void appendElementRefs(const QTreeWidgetItem* element, const QString& topic, const QString& type, QVector<TopicFieldRef>& refs);
  static void addField(QTreeWidgetItem* parent, const QString& name, const MessageFieldType& fieldType, const QString& path,
                       const QHash<QString, int>* lengths);
  static void addArrayElements(QTreeWidgetItem* item, const QString& name, const MessageFieldType& arrayType, const QString& path,
                               const QHash<QString, int>* lengths);
  static void addStatusChild(QTreeWidgetItem* parent, const QString& text, bool isError);
  static void replaceChildrenWithStatus(QTreeWidgetItem* item, const QString& text, bool isError);
  static QTreeWidgetItem* rootOf(QTreeWidgetItem* item);
  static void setDraggable(QTreeWidgetItem* item, bool draggable);
};

}  // namespace rqt_multiplot
