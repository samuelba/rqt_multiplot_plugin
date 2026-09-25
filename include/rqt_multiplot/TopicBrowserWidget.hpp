/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QMap>
#include <QShowEvent>
#include <QString>
#include <QWidget>

class QLineEdit;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace rqt_multiplot {

class BagTopicLoader;
class MessageTopicRegistry;
class TopicFieldTreeWidget;
class TopicSampleLoader;

class TopicBrowserWidget : public QWidget {
  Q_OBJECT
 public:
  explicit TopicBrowserWidget(QWidget* parent = nullptr);
  ~TopicBrowserWidget() override;

  void setLiveTopics(const QMap<QString, QString>& topics);
  void setBagTopics(const QString& fileName, const QMap<QString, QString>& topics);
  void setBagFile(const QString& fileName);
  void setFilterText(const QString& text);
  void refreshLiveTopics();

  QTreeWidget* getTopicList() const;
  TopicFieldTreeWidget* getFieldTree() const;
  QTreeWidgetItem* findTopicItem(const QString& key) const;
  static QString topicKey(bool fromBag, const QString& topic);

 protected:
  void showEvent(QShowEvent* event) override;

 private:
  struct TopicEntry {
    QString topic;
    QString type;
    bool fromBag = false;
  };

  QLineEdit* filterEdit_;
  QToolButton* refreshButton_;
  QTreeWidget* topicList_;
  TopicFieldTreeWidget* fieldTree_;
  QTreeWidgetItem* liveGroup_;
  QTreeWidgetItem* bagGroup_;
  MessageTopicRegistry* topicRegistry_;
  BagTopicLoader* bagTopicLoader_;
  QMap<QString, TopicEntry> checkedTopics_;
  QMap<QString, TopicSampleLoader*> samplers_;
  bool populating_;

  void populateGroup(QTreeWidgetItem* group, bool fromBag, const QMap<QString, QString>& topics);
  void addTopicItem(QTreeWidgetItem* group, const TopicEntry& entry);
  void applyFilter();
  void checkTopic(const QString& key, const TopicEntry& entry);
  void uncheckTopic(const QString& key);
  void releaseTopic(const QString& key);
  void loadFields(const QString& key, const TopicEntry& entry);
  void sampleArrayLengths(const QString& key, const TopicEntry& entry);

 private slots:
  void topicListItemChanged(QTreeWidgetItem* item, int column);
  void topicRegistryUpdateFinished();
  void bagTopicLoaderFinished();
  void bagTopicLoaderFailed(const QString& error);
};

}  // namespace rqt_multiplot
