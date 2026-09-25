#include <algorithm>
#include <memory>

#include <QApplication>
#include <QHeaderView>
#include <QImage>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <gtest/gtest.h>

#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/Theme.hpp"
#include "rqt_multiplot/TopicBrowserWidget.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"
#include "rqt_multiplot/TopicFieldTreeWidget.hpp"

namespace {

using rqt_multiplot::MessageFieldType;
using rqt_multiplot::Theme;
using rqt_multiplot::TopicBrowserWidget;
using rqt_multiplot::TopicFieldRef;
using rqt_multiplot::TopicFieldTreeWidget;

const QString kImuType = QStringLiteral("sensor_msgs/msg/Imu");
const QString kOdomType = QStringLiteral("nav_msgs/msg/Odometry");

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

QTreeWidgetItem* liveItem(const TopicBrowserWidget& browser, const QString& topic) {
  return browser.findTopicItem(TopicBrowserWidget::topicKey(false, topic));
}

MessageFieldType scalar(bool numeric) {
  MessageFieldType type;
  type.kind = MessageFieldType::Builtin;
  type.identifier = numeric ? "float64" : "string";
  type.isNumeric = numeric;
  return type;
}

MessageFieldType pointMessage() {
  MessageFieldType point;
  point.kind = MessageFieldType::Compound;
  point.identifier = "geometry_msgs/Point";
  point.members = {{"x", scalar(true)}, {"y", scalar(true)}, {"z", scalar(true)}};
  return point;
}

MessageFieldType poseMessage() {
  MessageFieldType values;
  values.kind = MessageFieldType::Array;
  values.identifier = "float64[]";
  values.isDynamicArray = true;
  values.elementType = std::make_shared<MessageFieldType>(scalar(true));

  MessageFieldType pose;
  pose.kind = MessageFieldType::Compound;
  pose.identifier = "test/Pose";
  pose.members = {{"frame", scalar(false)}, {"position", pointMessage()}, {"values", values}};
  return pose;
}

QTreeWidgetItem* childByText(QTreeWidgetItem* parent, const QString& text) {
  for (int i = 0; i < parent->childCount(); ++i) {
    if (parent->child(i)->text(0) == text) {
      return parent->child(i);
    }
  }
  return nullptr;
}

class TopicBrowserWidgetTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ensureApplication();
    browser_ = std::make_unique<TopicBrowserWidget>();
    browser_->setLiveTopics({{"/imu", kImuType}, {"/odom", kOdomType}});
  }

  std::unique_ptr<TopicBrowserWidget> browser_;
};

TEST_F(TopicBrowserWidgetTest, emptyFilterShowsAllTopics) {
  ASSERT_NE(liveItem(*browser_, "/imu"), nullptr);
  EXPECT_FALSE(liveItem(*browser_, "/imu")->isHidden());
  EXPECT_FALSE(liveItem(*browser_, "/odom")->isHidden());
  EXPECT_EQ(liveItem(*browser_, "/imu")->text(1), QString("Imu"));
}

TEST_F(TopicBrowserWidgetTest, substringFilterHidesOtherTopics) {
  browser_->setFilterText("IM");

  EXPECT_FALSE(liveItem(*browser_, "/imu")->isHidden());
  EXPECT_TRUE(liveItem(*browser_, "/odom")->isHidden());
}

TEST_F(TopicBrowserWidgetTest, filterMatchesType) {
  browser_->setFilterText("nav_msgs");

  EXPECT_TRUE(liveItem(*browser_, "/imu")->isHidden());
  EXPECT_FALSE(liveItem(*browser_, "/odom")->isHidden());
}

TEST_F(TopicBrowserWidgetTest, checkingAddsTreeNodeWithLoadingChild) {
  liveItem(*browser_, "/imu")->setCheckState(0, Qt::Checked);

  QTreeWidgetItem* node = browser_->getFieldTree()->topicItem(TopicBrowserWidget::topicKey(false, "/imu"));
  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->text(0), QString("/imu"));
  ASSERT_EQ(node->childCount(), 1);
  EXPECT_EQ(node->child(0)->text(0), QString("Loading..."));
}

TEST_F(TopicBrowserWidgetTest, uncheckingRemovesTreeNode) {
  liveItem(*browser_, "/imu")->setCheckState(0, Qt::Checked);
  liveItem(*browser_, "/imu")->setCheckState(0, Qt::Unchecked);

  EXPECT_TRUE(browser_->getFieldTree()->topicKeys().isEmpty());
}

TEST_F(TopicBrowserWidgetTest, checkedStateSurvivesFiltering) {
  liveItem(*browser_, "/odom")->setCheckState(0, Qt::Checked);

  browser_->setFilterText("imu");
  browser_->setFilterText(QString());

  EXPECT_EQ(liveItem(*browser_, "/odom")->checkState(0), Qt::Checked);
  EXPECT_TRUE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(false, "/odom")));
}

TEST_F(TopicBrowserWidgetTest, refreshKeepsCheckedTopicThatLeftTheGraph) {
  liveItem(*browser_, "/imu")->setCheckState(0, Qt::Checked);

  browser_->setLiveTopics({{"/odom", kOdomType}});

  ASSERT_NE(liveItem(*browser_, "/imu"), nullptr);
  EXPECT_EQ(liveItem(*browser_, "/imu")->checkState(0), Qt::Checked);
}

TEST_F(TopicBrowserWidgetTest, bagTopicsGetOwnGroupAndTreeSuffix) {
  browser_->setBagTopics("/tmp/run_042.mcap", {{"/imu", kImuType}});

  QTreeWidgetItem* bagItem = browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu"));
  ASSERT_NE(bagItem, nullptr);
  EXPECT_EQ(bagItem->parent()->text(0), QString("Bag: run_042.mcap"));
  EXPECT_FALSE(bagItem->parent()->isHidden());

  bagItem->setCheckState(0, Qt::Checked);
  EXPECT_EQ(browser_->getFieldTree()->topicItem(TopicBrowserWidget::topicKey(true, "/imu"))->text(0), QString("/imu [bag]"));
  EXPECT_FALSE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(false, "/imu")));
}

TEST_F(TopicBrowserWidgetTest, newBagDropsCheckedTopicsItDoesNotContain) {
  browser_->setBagTopics("/tmp/a.mcap", {{"/imu", kImuType}, {"/odom", kOdomType}});
  browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu"))->setCheckState(0, Qt::Checked);
  browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/odom"))->setCheckState(0, Qt::Checked);

  browser_->setBagTopics("/tmp/b.mcap", {{"/odom", kOdomType}});

  EXPECT_FALSE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(true, "/imu")));
  EXPECT_TRUE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(true, "/odom")));
  EXPECT_EQ(browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/odom"))->checkState(0), Qt::Checked);
}

TEST(TopicFieldTreeWidget, nameColumnIsInteractiveAndAbsorbsWidthChanges) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.resize(300, 200);
  tree.show();
  QApplication::processEvents();

  QHeaderView* header = tree.header();
  EXPECT_EQ(header->sectionResizeMode(0), QHeaderView::Interactive);
  const int nameWidth = header->sectionSize(0);
  const int typeWidth = header->sectionSize(1);
  EXPECT_GT(nameWidth, typeWidth);

  tree.resize(400, 200);
  QApplication::processEvents();

  EXPECT_EQ(header->sectionSize(0), nameWidth + 100);
  EXPECT_EQ(header->sectionSize(1), typeWidth);
}

TEST(TopicBrowserWidget, topicColumnIsInteractiveAndWiderThanType) {
  ensureApplication();
  TopicBrowserWidget browser;
  browser.resize(300, 400);
  browser.show();
  QApplication::processEvents();

  QHeaderView* header = browser.getTopicList()->header();
  EXPECT_EQ(header->sectionResizeMode(0), QHeaderView::Interactive);
  EXPECT_GT(header->sectionSize(0), header->sectionSize(1));
}

QImage renderUncheckedIndicator(QWidget* widget, const QPalette& palette) {
  QImage image(32, 32, QImage::Format_ARGB32_Premultiplied);
  image.fill(palette.color(QPalette::Base));
  QPainter painter(&image);
  QStyleOptionViewItem option;
  option.rect = QRect(8, 8, 16, 16);
  option.state = QStyle::State_Enabled | QStyle::State_Active | QStyle::State_Off;
  option.palette = palette;
  widget->style()->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, &painter, widget);
  return image;
}

int brightestGray(const QImage& image) {
  int brightest = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      brightest = std::max(brightest, qGray(image.pixel(x, y)));
    }
  }
  return brightest;
}

TEST(TopicBrowserWidget, darkThemeCheckboxBorderIsVisible) {
  ensureApplication();
  TopicBrowserWidget browser;
  const QPalette palette = Theme::palette(Theme::Id::Dark);
  Theme::apply(&browser, Theme::Id::Dark);

  const QImage image = renderUncheckedIndicator(browser.getTopicList()->viewport(), palette);

  EXPECT_GE(brightestGray(image), 180);
  EXPECT_LE(qGray(image.pixel(16, 16)), 90);

  Theme::apply(&browser, Theme::Id::Light);
}

TEST(TopicBrowserWidget, lightThemeCheckboxKeepsLightFill) {
  ensureApplication();
  TopicBrowserWidget browser;
  const QPalette palette = Theme::palette(Theme::Id::Light);
  Theme::apply(&browser, Theme::Id::Light);

  const QImage image = renderUncheckedIndicator(browser.getTopicList()->viewport(), palette);

  EXPECT_GE(qGray(image.pixel(16, 16)), 200);
}

TEST(TopicFieldTreeWidget, refsForMultiSelectionExpandNodesAndArrays) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());
  QTreeWidgetItem* root = tree.topicItem("live:/pose");
  QTreeWidgetItem* position = childByText(root, "position");
  ASSERT_NE(position, nullptr);

  const QVector<TopicFieldRef> refs =
      TopicFieldTreeWidget::refsForItems({position, childByText(position, "x"), childByText(root, "values")});

  const QVector<TopicFieldRef> expected = {{"/pose", "test/msg/Pose", "position/x"},
                                           {"/pose", "test/msg/Pose", "position/y"},
                                           {"/pose", "test/msg/Pose", "position/z"},
                                           {"/pose", "test/msg/Pose", "values/*"}};
  EXPECT_EQ(refs, expected);
}

TEST(TopicFieldTreeWidget, stringFieldsAreNotDraggable) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());

  QTreeWidgetItem* frame = childByText(tree.topicItem("live:/pose"), "frame");
  ASSERT_NE(frame, nullptr);
  EXPECT_FALSE(frame->flags().testFlag(Qt::ItemIsDragEnabled));
  EXPECT_TRUE(TopicFieldTreeWidget::refsForItems({frame}).isEmpty());
}

TEST(TopicFieldTreeWidget, topicNodeDragsAllNumericLeaves) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());

  EXPECT_EQ(TopicFieldTreeWidget::refsForItems({tree.topicItem("live:/pose")}).count(), 3);
}

TEST(TopicFieldTreeWidget, fixedArrayListsIndexedElementsFromDefinition) {
  ensureApplication();
  MessageFieldType covariance;
  covariance.kind = MessageFieldType::Array;
  covariance.identifier = "float64[3]";
  covariance.arraySize = 3;
  covariance.elementType = std::make_shared<MessageFieldType>(scalar(true));
  MessageFieldType message;
  message.kind = MessageFieldType::Compound;
  message.members = {{"covariance", covariance}};

  TopicFieldTreeWidget tree;
  tree.addTopic("live:/cov", "/cov", "test/msg/Cov", false);
  tree.setTopicDefinition("live:/cov", message);

  QTreeWidgetItem* array = childByText(tree.topicItem("live:/cov"), "covariance");
  ASSERT_NE(array, nullptr);
  ASSERT_EQ(array->childCount(), 3);
  EXPECT_EQ(array->child(1)->text(0), QString("covariance[1]"));
  EXPECT_EQ(TopicFieldTreeWidget::refsForItems({array->child(1)}).value(0).field, QString("covariance/1"));
  EXPECT_EQ(TopicFieldTreeWidget::refsForItems({array}).value(0).field, QString("covariance/*"));
}

class TopicFieldTreeArrayTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ensureApplication();
    MessageFieldType poses;
    poses.kind = MessageFieldType::Array;
    poses.identifier = "test/Pose[]";
    poses.isDynamicArray = true;
    poses.elementType = std::make_shared<MessageFieldType>(poseMessage());
    MessageFieldType message;
    message.kind = MessageFieldType::Compound;
    message.members = {{"poses", poses}};

    tree_.addTopic("live:/poses", "/poses", "test/msg/PoseArray", false);
    tree_.setTopicDefinition("live:/poses", message);
  }

  QTreeWidgetItem* posesItem() { return childByText(tree_.topicItem("live:/poses"), "poses"); }

  static QStringList fields(const QVector<TopicFieldRef>& refs) {
    QStringList result;
    for (const auto& ref : refs) {
      result.append(ref.field);
    }
    return result;
  }

  TopicFieldTreeWidget tree_;
};

TEST_F(TopicFieldTreeArrayTest, dynamicArrayWaitsForSample) {
  ASSERT_NE(posesItem(), nullptr);
  ASSERT_EQ(posesItem()->childCount(), 1);
  EXPECT_FALSE(posesItem()->child(0)->flags().testFlag(Qt::ItemIsDragEnabled));
  EXPECT_EQ(fields(TopicFieldTreeWidget::refsForItems({posesItem()})),
            QStringList({"poses/*/position/x", "poses/*/position/y", "poses/*/position/z"}));
}

TEST_F(TopicFieldTreeArrayTest, sampledLengthsListElementsAndNestedArrays) {
  tree_.setTopicArrayLengths("live:/poses", {{"poses", 2}, {"poses/0/values", 2}});

  ASSERT_EQ(posesItem()->childCount(), 2);
  QTreeWidgetItem* second = posesItem()->child(1);
  EXPECT_EQ(second->text(0), QString("poses[1]"));
  EXPECT_EQ(fields(TopicFieldTreeWidget::refsForItems({second})),
            QStringList({"poses/1/position/x", "poses/1/position/y", "poses/1/position/z"}));
  EXPECT_EQ(fields(TopicFieldTreeWidget::refsForItems({childByText(childByText(second, "position"), "x")})),
            QStringList({"poses/1/position/x"}));

  QTreeWidgetItem* values = childByText(posesItem()->child(0), "values");
  ASSERT_NE(values, nullptr);
  ASSERT_EQ(values->childCount(), 2);
  EXPECT_EQ(fields(TopicFieldTreeWidget::refsForItems({values->child(1)})), QStringList({"poses/0/values/1"}));
  EXPECT_EQ(fields(TopicFieldTreeWidget::refsForItems({values})), QStringList({"poses/0/values/*"}));
}

TEST_F(TopicFieldTreeArrayTest, sampledLengthsKeepExpandedItems) {
  tree_.setTopicArrayLengths("live:/poses", {{"poses", 1}});
  posesItem()->setExpanded(true);
  posesItem()->child(0)->setExpanded(true);

  tree_.setTopicArrayLengths("live:/poses", {{"poses", 2}});

  EXPECT_TRUE(posesItem()->isExpanded());
  EXPECT_TRUE(posesItem()->child(0)->isExpanded());
  EXPECT_FALSE(posesItem()->child(1)->isExpanded());
}

TEST_F(TopicFieldTreeArrayTest, longArraysAreCappedWithHint) {
  tree_.setTopicArrayLengths("live:/poses", {{"poses", rqt_multiplot::kMaxArrayElementsShown + 5}});

  ASSERT_EQ(posesItem()->childCount(), rqt_multiplot::kMaxArrayElementsShown + 1);
  EXPECT_TRUE(posesItem()->child(rqt_multiplot::kMaxArrayElementsShown)->text(0).contains("5"));
}

TEST(TopicFieldTreeWidget, errorReplacesLoadingChild) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);

  tree.setTopicError("live:/pose", "boom");

  QTreeWidgetItem* root = tree.topicItem("live:/pose");
  ASSERT_EQ(root->childCount(), 1);
  EXPECT_EQ(root->child(0)->text(0), QString("boom"));
}

}  // namespace
