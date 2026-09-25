/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TopicBrowserWidget.hpp"

#include <QDebug>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSizePolicy>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "rqt_multiplot/BagTopicLoader.hpp"
#include "rqt_multiplot/MessageDefinitionLoader.hpp"
#include "rqt_multiplot/MessageTopicRegistry.hpp"
#include "rqt_multiplot/PackageResource.hpp"
#include "rqt_multiplot/PlotSplitter.hpp"
#include "rqt_multiplot/TopicFieldTreeWidget.hpp"
#include "rqt_multiplot/TopicSampleLoader.hpp"

namespace rqt_multiplot {

namespace {

constexpr int kKeyRole = Qt::UserRole;
constexpr int kTopicRole = Qt::UserRole + 1;
constexpr int kTypeRole = Qt::UserRole + 2;
constexpr int kFromBagRole = Qt::UserRole + 3;
constexpr int kHeadingMarginPx = 8;
constexpr int kHeadingInnerMarginPx = 4;
constexpr int kHeadingSpacingPx = 6;
constexpr int kListStretch = 1;
constexpr int kTreeStretch = 2;

QTreeWidgetItem* makeGroupItem(QTreeWidget* list, const QString& text) {
  auto* group = new QTreeWidgetItem(list);
  group->setText(0, text);
  group->setFlags(Qt::ItemIsEnabled);
  QFont font = group->font(0);
  font.setBold(true);
  group->setFont(0, font);
  group->setFirstColumnSpanned(true);
  group->setExpanded(true);
  return group;
}

void toggleTopicCheckState(QTreeWidgetItem* item) {
  if (item->parent() != nullptr) {
    item->setCheckState(0, (item->checkState(0) == Qt::Checked) ? Qt::Unchecked : Qt::Checked);
  }
}

}  // namespace

TopicBrowserWidget::TopicBrowserWidget(QWidget* parent)
    : QWidget(parent),
      filterEdit_(new QLineEdit(this)),
      refreshButton_(new QToolButton(this)),
      topicList_(new QTreeWidget(this)),
      fieldTree_(new TopicFieldTreeWidget(this)),
      liveGroup_(nullptr),
      bagGroup_(nullptr),
      topicRegistry_(new MessageTopicRegistry(this)),
      bagTopicLoader_(new BagTopicLoader(this)),
      populating_(false) {
  setObjectName(QStringLiteral("topicBrowserWidget"));

  auto* heading = new QLabel(tr("Topics"), this);
  heading->setObjectName(QStringLiteral("topicBrowserHeading"));
  QFont headingFont = heading->font();
  headingFont.setBold(true);
  heading->setFont(headingFont);

  filterEdit_->setObjectName(QStringLiteral("topicBrowserFilter"));
  filterEdit_->setPlaceholderText(tr("Filter topics..."));
  filterEdit_->setClearButtonEnabled(true);

  refreshButton_->setObjectName(QStringLiteral("topicBrowserRefresh"));
  refreshButton_->setToolTip(tr("Refresh live topics"));
  refreshButton_->setAutoRaise(true);
  setThemeIcon(refreshButton_, QStringLiteral("resource/renew.svg"));

  auto* filterRow = new QHBoxLayout();
  filterRow->setContentsMargins(0, 0, 0, 0);
  filterRow->addWidget(filterEdit_, 1);
  filterRow->addWidget(refreshButton_);

  topicList_->setObjectName(QStringLiteral("topicBrowserList"));
  topicList_->setColumnCount(2);
  topicList_->setHeaderLabels({tr("Topic"), tr("Type")});
  topicList_->header()->setMinimumSectionSize(150);
  topicList_->setIndentation(8);
  makeLeadingColumnGrow(topicList_);
  topicList_->setFrameShape(QFrame::NoFrame);
  topicList_->setExpandsOnDoubleClick(false);
  liveGroup_ = makeGroupItem(topicList_, tr("Live"));
  bagGroup_ = makeGroupItem(topicList_, tr("Bag"));
  bagGroup_->setHidden(true);

  auto* splitter = new PlotSplitter(Qt::Vertical, this);
  splitter->setObjectName(QStringLiteral("topicBrowserSplitter"));
  splitter->addWidget(topicList_);
  splitter->addWidget(fieldTree_);
  splitter->setStretchFactor(0, kListStretch);
  splitter->setStretchFactor(1, kTreeStretch);
  splitter->setChildrenCollapsible(false);

  setMinimumWidth(0);
  setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

  auto* layout = new QVBoxLayout(this);
  layout->setSizeConstraint(QLayout::SetNoConstraint);
  layout->setContentsMargins(kHeadingMarginPx, kHeadingMarginPx, kHeadingInnerMarginPx, kHeadingInnerMarginPx);
  layout->setSpacing(kHeadingSpacingPx);
  layout->addWidget(heading);
  layout->addLayout(filterRow);
  layout->addWidget(splitter, 1);

  connect(filterEdit_, &QLineEdit::textChanged, this, [this]() { applyFilter(); });
  connect(refreshButton_, &QToolButton::clicked, this, &TopicBrowserWidget::refreshLiveTopics);
  connect(topicList_, &QTreeWidget::itemChanged, this, &TopicBrowserWidget::topicListItemChanged);
  connect(topicList_, &QTreeWidget::itemDoubleClicked, this, &toggleTopicCheckState);
  connect(topicRegistry_, &MessageTopicRegistry::updateFinished, this, &TopicBrowserWidget::topicRegistryUpdateFinished);
  connect(bagTopicLoader_, &BagTopicLoader::loadingFinished, this, &TopicBrowserWidget::bagTopicLoaderFinished);
  connect(bagTopicLoader_, &BagTopicLoader::loadingFailed, this, &TopicBrowserWidget::bagTopicLoaderFailed);

  applyFilter();
}

TopicBrowserWidget::~TopicBrowserWidget() = default;

void TopicBrowserWidget::setLiveTopics(const QMap<QString, QString>& topics) {
  populateGroup(liveGroup_, false, topics);
}

void TopicBrowserWidget::setBagTopics(const QString& fileName, const QMap<QString, QString>& topics) {
  bagGroup_->setText(0, tr("Bag: %1").arg(QFileInfo(fileName).fileName()));
  bagGroup_->setToolTip(0, fileName);
  populateGroup(bagGroup_, true, topics);
}

void TopicBrowserWidget::setBagFile(const QString& fileName) {
  bagTopicLoader_->load(fileName);
}

void TopicBrowserWidget::setFilterText(const QString& text) {
  filterEdit_->setText(text);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
void TopicBrowserWidget::refreshLiveTopics() {
  MessageTopicRegistry::update();
}

QTreeWidget* TopicBrowserWidget::getTopicList() const {
  return topicList_;
}

TopicFieldTreeWidget* TopicBrowserWidget::getFieldTree() const {
  return fieldTree_;
}

QTreeWidgetItem* TopicBrowserWidget::findTopicItem(const QString& key) const {
  for (QTreeWidgetItem* group : {liveGroup_, bagGroup_}) {
    for (int i = 0; i < group->childCount(); ++i) {
      if (group->child(i)->data(0, kKeyRole).toString() == key) {
        return group->child(i);
      }
    }
  }
  return nullptr;
}

QString TopicBrowserWidget::topicKey(bool fromBag, const QString& topic) {
  return (fromBag ? QStringLiteral("bag:") : QStringLiteral("live:")) + topic;
}

void TopicBrowserWidget::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  refreshLiveTopics();
}

void TopicBrowserWidget::populateGroup(QTreeWidgetItem* group, bool fromBag, const QMap<QString, QString>& topics) {
  QMap<QString, TopicEntry> entries;
  for (auto it = topics.constBegin(); it != topics.constEnd(); ++it) {
    entries.insert(topicKey(fromBag, it.key()), TopicEntry{it.key(), it.value(), fromBag});
  }

  // Checked live topics stay listed after they leave the graph, so they can still be unchecked.
  for (auto it = checkedTopics_.begin(); it != checkedTopics_.end();) {
    if ((it->fromBag != fromBag) || entries.contains(it.key())) {
      ++it;
    } else if (fromBag) {
      releaseTopic(it.key());
      it = checkedTopics_.erase(it);
    } else {
      entries.insert(it.key(), it.value());
      ++it;
    }
  }

  populating_ = true;
  qDeleteAll(group->takeChildren());
  for (const auto& entry : entries) {
    addTopicItem(group, entry);
  }
  group->setExpanded(true);
  populating_ = false;

  applyFilter();
}

void TopicBrowserWidget::addTopicItem(QTreeWidgetItem* group, const TopicEntry& entry) {
  const QString key = topicKey(entry.fromBag, entry.topic);
  auto* item = new QTreeWidgetItem(group);
  item->setText(0, entry.topic);
  item->setText(1, shortTypeName(entry.type));
  item->setToolTip(0, entry.topic);
  item->setToolTip(1, entry.type);
  item->setData(0, kKeyRole, key);
  item->setData(0, kTopicRole, entry.topic);
  item->setData(0, kTypeRole, entry.type);
  item->setData(0, kFromBagRole, entry.fromBag);
  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
  item->setCheckState(0, checkedTopics_.contains(key) ? Qt::Checked : Qt::Unchecked);
}

void TopicBrowserWidget::applyFilter() {
  const QString filter = filterEdit_->text().trimmed();
  for (QTreeWidgetItem* group : {liveGroup_, bagGroup_}) {
    int visibleCount = 0;
    for (int i = 0; i < group->childCount(); ++i) {
      QTreeWidgetItem* item = group->child(i);
      const bool matches = filter.isEmpty() || item->data(0, kTopicRole).toString().contains(filter, Qt::CaseInsensitive) ||
                           item->data(0, kTypeRole).toString().contains(filter, Qt::CaseInsensitive);
      item->setHidden(!matches);
      visibleCount += matches ? 1 : 0;
    }
    group->setHidden(visibleCount == 0);
  }
}

void TopicBrowserWidget::checkTopic(const QString& key, const TopicEntry& entry) {
  if (checkedTopics_.contains(key)) {
    return;
  }
  checkedTopics_.insert(key, entry);
  fieldTree_->addTopic(key, entry.topic, entry.type, entry.fromBag);
  loadFields(key, entry);
}

void TopicBrowserWidget::uncheckTopic(const QString& key) {
  checkedTopics_.remove(key);
  releaseTopic(key);
}

void TopicBrowserWidget::releaseTopic(const QString& key) {
  fieldTree_->removeTopic(key);
  delete samplers_.take(key);
}

void TopicBrowserWidget::loadFields(const QString& key, const TopicEntry& entry) {
  auto* loader = new MessageDefinitionLoader(this);
  connect(loader, &MessageDefinitionLoader::loadingFinished, this, [this, loader, key, entry]() {
    const MessageFieldType definition = loader->getDefinition();
    fieldTree_->setTopicDefinition(key, definition);
    if (containsDynamicArray(definition)) {
      sampleArrayLengths(key, entry);
    }
    loader->deleteLater();
  });
  connect(loader, &MessageDefinitionLoader::loadingFailed, this, [this, loader, key](const QString& error) {
    fieldTree_->setTopicError(key, tr("Fields unavailable: %1").arg(error));
    loader->deleteLater();
  });
  loader->load(entry.type);
}

void TopicBrowserWidget::sampleArrayLengths(const QString& key, const TopicEntry& entry) {
  if (!checkedTopics_.contains(key) || samplers_.contains(key)) {
    return;
  }

  auto* sampler = new TopicSampleLoader(this);
  samplers_.insert(key, sampler);
  connect(sampler, &TopicSampleLoader::sampled, this,
          [this, key](const QHash<QString, int>& lengths) { fieldTree_->setTopicArrayLengths(key, lengths); });
  connect(sampler, &TopicSampleLoader::samplingFailed, this, [this, key, entry](const QString& error) {
    qWarning() << "Failed to sample array lengths of" << entry.topic << ":" << error;
    if (entry.fromBag) {
      fieldTree_->setTopicArrayLengths(key, {});
    }
  });

  if (entry.fromBag) {
    sampler->sampleBag(bagTopicLoader_->getFileName(), entry.topic, entry.type);
  } else {
    sampler->sampleLive(entry.topic);
  }
}

void TopicBrowserWidget::topicListItemChanged(QTreeWidgetItem* item, int column) {
  if (populating_ || (column != 0) || (item->parent() == nullptr)) {
    return;
  }

  const QString key = item->data(0, kKeyRole).toString();
  if (item->checkState(0) == Qt::Checked) {
    checkTopic(key,
               TopicEntry{item->data(0, kTopicRole).toString(), item->data(0, kTypeRole).toString(), item->data(0, kFromBagRole).toBool()});
  } else {
    uncheckTopic(key);
  }
}

void TopicBrowserWidget::topicRegistryUpdateFinished() {
  setLiveTopics(MessageTopicRegistry::getTopics());
}

void TopicBrowserWidget::bagTopicLoaderFinished() {
  setBagTopics(bagTopicLoader_->getFileName(), bagTopicLoader_->getTopics());
}

void TopicBrowserWidget::bagTopicLoaderFailed(const QString& error) {
  qWarning() << "Failed to read topics from bag [file://" << bagTopicLoader_->getFileName() << "]:" << error;
}

}  // namespace rqt_multiplot
