#include <algorithm>
#include <memory>

#include <QApplication>
#include <QHeaderView>
#include <QImage>
#include <QListWidget>
#include <QMimeData>
#include <QPainter>
#include <QStyle>
#include <QStyleOption>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <gtest/gtest.h>

#include "rqt_multiplot/DiagnosticKeySampler.hpp"
#include "rqt_multiplot/MessageDefinitionLoader.hpp"
#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/MessageTopicRegistry.hpp"
#include "rqt_multiplot/Theme.hpp"
#include "rqt_multiplot/TopicBrowserWidget.hpp"
#include "rqt_multiplot/TopicDropDialog.hpp"
#include "rqt_multiplot/TopicFieldMime.hpp"
#include "rqt_multiplot/TopicFieldTreeWidget.hpp"

namespace {

using rqt_multiplot::DiagnosticKeySampler;
using rqt_multiplot::MessageDefinitionLoader;
using rqt_multiplot::MessageFieldType;
using rqt_multiplot::Theme;
using rqt_multiplot::TopicBrowserWidget;
using rqt_multiplot::TopicDropDialog;
using rqt_multiplot::TopicFieldRef;
using rqt_multiplot::TopicFieldTreeWidget;

const QString kImuType = QStringLiteral("sensor_msgs/msg/Imu");
const QString kOdomType = QStringLiteral("nav_msgs/msg/Odometry");
const QString kDiagnosticType = QStringLiteral("diagnostic_msgs/msg/DiagnosticArray");

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

DiagnosticKeySampler* diagnosticSamplerAfterLoad(TopicBrowserWidget* browser) {
  auto* loader = browser->findChild<MessageDefinitionLoader*>();
  if (loader == nullptr) {
    return nullptr;
  }
  loader->wait();
  QApplication::processEvents();
  return browser->findChild<DiagnosticKeySampler*>();
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
  browser_->setBagTopics({QStringLiteral("/tmp/run_042.mcap")}, {{"/imu", kImuType}});

  QTreeWidgetItem* bagItem = browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu"));
  ASSERT_NE(bagItem, nullptr);
  EXPECT_EQ(bagItem->parent()->text(0), QString("Bags (1)"));
  EXPECT_EQ(bagItem->parent()->toolTip(0), QString("/tmp/run_042.mcap"));
  EXPECT_FALSE(bagItem->parent()->isHidden());

  bagItem->setCheckState(0, Qt::Checked);
  EXPECT_EQ(browser_->getFieldTree()->topicItem(TopicBrowserWidget::topicKey(true, "/imu"))->text(0), QString("/imu [bag]"));
  EXPECT_FALSE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(false, "/imu")));
}

TEST_F(TopicBrowserWidgetTest, newBagDropsCheckedTopicsItDoesNotContain) {
  browser_->setBagTopics({QStringLiteral("/tmp/a.mcap")}, {{"/imu", kImuType}, {"/odom", kOdomType}});
  browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu"))->setCheckState(0, Qt::Checked);
  browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/odom"))->setCheckState(0, Qt::Checked);

  browser_->setBagTopics({QStringLiteral("/tmp/b.mcap")}, {{"/odom", kOdomType}});

  EXPECT_FALSE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(true, "/imu")));
  EXPECT_TRUE(browser_->getFieldTree()->hasTopic(TopicBrowserWidget::topicKey(true, "/odom")));
  EXPECT_EQ(browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/odom"))->checkState(0), Qt::Checked);
}

TEST_F(TopicBrowserWidgetTest, diagnosticSamplingFollowsBrowserVisibility) {
  browser_->setLiveTopics({{"/diagnostics", kDiagnosticType}});
  browser_->resize(400, 300);
  browser_->show();
  rqt_multiplot::MessageTopicRegistry::wait();
  QApplication::processEvents();

  ASSERT_NE(liveItem(*browser_, "/diagnostics"), nullptr);
  liveItem(*browser_, "/diagnostics")->setCheckState(0, Qt::Checked);
  DiagnosticKeySampler* sampler = diagnosticSamplerAfterLoad(browser_.get());
  ASSERT_NE(sampler, nullptr);
  EXPECT_TRUE(sampler->isSamplingLive());

  browser_->hide();
  QApplication::processEvents();
  EXPECT_FALSE(sampler->isSamplingLive());

  browser_->show();
  QApplication::processEvents();
  EXPECT_TRUE(sampler->isSamplingLive());
}

TEST_F(TopicBrowserWidgetTest, hiddenDiagnosticTopicSubscribesWhenBrowserIsShown) {
  browser_->setLiveTopics({{"/diagnostics", kDiagnosticType}});

  liveItem(*browser_, "/diagnostics")->setCheckState(0, Qt::Checked);
  DiagnosticKeySampler* sampler = diagnosticSamplerAfterLoad(browser_.get());
  ASSERT_NE(sampler, nullptr);
  EXPECT_FALSE(sampler->isSamplingLive());

  browser_->resize(400, 300);
  browser_->show();
  QApplication::processEvents();
  EXPECT_TRUE(sampler->isSamplingLive());

  liveItem(*browser_, "/diagnostics")->setCheckState(0, Qt::Unchecked);
  QApplication::processEvents();
  EXPECT_EQ(browser_->findChild<DiagnosticKeySampler*>(), nullptr);
}

TEST_F(TopicBrowserWidgetTest, uncheckingHiddenDiagnosticTopicStaysUnsubscribed) {
  browser_->setLiveTopics({{"/diagnostics", kDiagnosticType}});
  liveItem(*browser_, "/diagnostics")->setCheckState(0, Qt::Checked);
  ASSERT_NE(diagnosticSamplerAfterLoad(browser_.get()), nullptr);

  liveItem(*browser_, "/diagnostics")->setCheckState(0, Qt::Unchecked);
  browser_->resize(400, 300);
  browser_->show();
  QApplication::processEvents();

  EXPECT_EQ(browser_->findChild<DiagnosticKeySampler*>(), nullptr);
}

TEST_F(TopicBrowserWidgetTest, addBagKeepsTopicsFromEarlierFiles) {
  browser_->setBagTopics({QStringLiteral("/tmp/a.mcap")}, {{"/imu", kImuType}});

  browser_->setBagTopics({QStringLiteral("/tmp/b.mcap")}, {{"/odom", kOdomType}}, false);

  EXPECT_NE(browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu")), nullptr);
  EXPECT_NE(browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/odom")), nullptr);
  EXPECT_EQ(browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu"))->parent()->text(0), QString("Bags (2)"));
  EXPECT_EQ(browser_->findTopicItem(TopicBrowserWidget::topicKey(true, "/imu"))->parent()->toolTip(0), QString("/tmp/a.mcap\n/tmp/b.mcap"));
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

TEST(TopicDropDialog, darkThemeCheckboxBorderIsVisible) {
  ensureApplication();
  Theme::apply(nullptr, Theme::Id::Dark);
  TopicDropDialog dialog(nullptr, {{"/pose", "test/msg/Pose", "x"}}, rqt_multiplot::topicMetricRefs("/pose", "test/msg/Pose", false));
  auto* metrics = dialog.findChild<QListWidget*>("topicDropMetricsList");
  ASSERT_NE(metrics, nullptr);

  const QImage image = renderUncheckedIndicator(metrics->viewport(), Theme::palette(Theme::Id::Dark));

  EXPECT_EQ(dialog.palette().color(QPalette::Window), Theme::palette(Theme::Id::Dark).color(QPalette::Window));
  EXPECT_GE(brightestGray(image), 180);
  Theme::apply(nullptr, Theme::Id::Light);
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

class MimeTree : public TopicFieldTreeWidget {
 public:
  using TopicFieldTreeWidget::mimeData;
};

MessageFieldType stampedPointMessage() {
  MessageFieldType stamp = scalar(true);
  stamp.identifier = "builtin_interfaces/Time";
  MessageFieldType header;
  header.kind = MessageFieldType::Compound;
  header.identifier = "std_msgs/Header";
  header.members = {{"stamp", stamp}, {"frame_id", scalar(false)}};
  MessageFieldType message;
  message.kind = MessageFieldType::Compound;
  message.identifier = "geometry_msgs/PointStamped";
  message.members = {{"header", header}, {"point", pointMessage()}};
  return message;
}

TEST(TopicFieldTreeWidget, topicMetricsGroupOffersDelayOnlyWithHeader) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());
  tree.addTopic("live:/point", "/point", "geometry_msgs/msg/PointStamped", false);
  tree.setTopicDefinition("live:/point", stampedPointMessage());

  QTreeWidgetItem* poseMetrics = childByText(tree.topicItem("live:/pose"), "Topic metrics");
  QTreeWidgetItem* pointMetrics = childByText(tree.topicItem("live:/point"), "Topic metrics");
  ASSERT_NE(poseMetrics, nullptr);
  ASSERT_NE(pointMetrics, nullptr);
  EXPECT_NE(childByText(poseMetrics, "rate"), nullptr);
  EXPECT_NE(childByText(poseMetrics, "bandwidth"), nullptr);
  EXPECT_EQ(childByText(poseMetrics, "delay_mean"), nullptr);
  EXPECT_NE(childByText(pointMetrics, "delay_mean"), nullptr);
  EXPECT_EQ(pointMetrics->childCount(), rqt_multiplot::kTopicMetricCount);
}

TEST(TopicFieldTreeWidget, metricLeafAndGroupGiveMetricRefs) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());
  QTreeWidgetItem* metrics = childByText(tree.topicItem("live:/pose"), "Topic metrics");
  ASSERT_NE(metrics, nullptr);
  QTreeWidgetItem* rate = childByText(metrics, "rate");
  ASSERT_NE(rate, nullptr);
  EXPECT_TRUE(rate->flags().testFlag(Qt::ItemIsDragEnabled));
  EXPECT_TRUE(metrics->flags().testFlag(Qt::ItemIsDragEnabled));

  const QVector<TopicFieldRef> rateRefs = TopicFieldTreeWidget::refsForItems({rate});
  ASSERT_EQ(rateRefs.count(), 1);
  EXPECT_EQ(rateRefs[0], TopicFieldRef::forMetric("/pose", "test/msg/Pose", rqt_multiplot::TopicMetric::Rate));

  EXPECT_EQ(TopicFieldTreeWidget::refsForItems({metrics, rate}), rqt_multiplot::topicMetricRefs("/pose", "test/msg/Pose", false));
}

TEST(TopicFieldTreeWidget, topicRootDragCarriesFieldsAndOfferedMetrics) {
  ensureApplication();
  MimeTree tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());
  QTreeWidgetItem* root = tree.topicItem("live:/pose");

  const std::unique_ptr<QMimeData> rootData(tree.mimeData({root}));
  const std::unique_ptr<QMimeData> fieldData(tree.mimeData({childByText(root, "position")}));

  ASSERT_NE(rootData, nullptr);
  ASSERT_TRUE(rootData->hasFormat(rqt_multiplot::kTopicRootMimeType));
  EXPECT_EQ(rqt_multiplot::decodeTopicFields(rootData->data(rqt_multiplot::kTopicRootMimeType)),
            rqt_multiplot::topicMetricRefs("/pose", "test/msg/Pose", false));
  const QVector<TopicFieldRef> expectedFields = {{"/pose", "test/msg/Pose", "position/x"},
                                                 {"/pose", "test/msg/Pose", "position/y"},
                                                 {"/pose", "test/msg/Pose", "position/z"},
                                                 {"/pose", "test/msg/Pose", "values/*"}};
  EXPECT_EQ(rqt_multiplot::decodeTopicFields(rootData->data(rqt_multiplot::kTopicFieldsMimeType)), expectedFields);
  ASSERT_NE(fieldData, nullptr);
  EXPECT_FALSE(fieldData->hasFormat(rqt_multiplot::kTopicRootMimeType));
}

TEST(TopicFieldTreeWidget, topicWithoutNumericFieldsIsStillDraggableForMetrics) {
  ensureApplication();
  MessageFieldType message;
  message.kind = MessageFieldType::Compound;
  message.members = {{"data", scalar(false)}};
  MimeTree tree;
  tree.addTopic("live:/chatter", "/chatter", "std_msgs/msg/String", false);
  tree.setTopicDefinition("live:/chatter", message);
  QTreeWidgetItem* root = tree.topicItem("live:/chatter");

  EXPECT_TRUE(root->flags().testFlag(Qt::ItemIsDragEnabled));
  const std::unique_ptr<QMimeData> data(tree.mimeData({root}));
  ASSERT_NE(data, nullptr);
  EXPECT_TRUE(rqt_multiplot::decodeTopicFields(data->data(rqt_multiplot::kTopicFieldsMimeType)).isEmpty());
  EXPECT_FALSE(rqt_multiplot::decodeTopicFields(data->data(rqt_multiplot::kTopicRootMimeType)).isEmpty());
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

TEST_F(TopicFieldTreeArrayTest, expandedRefsListEachElementAndNestedArrayWildcards) {
  tree_.setTopicArrayLengths("live:/poses", {{"poses", 2}});

  EXPECT_EQ(fields(TopicFieldTreeWidget::expandedRefsForItems({posesItem()})),
            QStringList({"poses/0/position/x", "poses/0/position/y", "poses/0/position/z", "poses/0/values/*", "poses/1/position/x",
                         "poses/1/position/y", "poses/1/position/z", "poses/1/values/*"}));
}

TEST_F(TopicFieldTreeArrayTest, expandedRefsFallBackToWildcardsForUnsampledArray) {
  EXPECT_EQ(TopicFieldTreeWidget::expandedRefsForItems({posesItem()}), TopicFieldTreeWidget::refsForItems({posesItem()}));
}

TEST_F(TopicFieldTreeArrayTest, mimeDataCarriesExpandedRefsOnlyForSampledArrays) {
  MimeTree tree;
  MessageFieldType poses;
  poses.kind = MessageFieldType::Array;
  poses.isDynamicArray = true;
  poses.elementType = std::make_shared<MessageFieldType>(pointMessage());
  MessageFieldType message;
  message.kind = MessageFieldType::Compound;
  message.members = {{"poses", poses}};
  tree.addTopic("live:/poses", "/poses", "test/msg/PointArray", false);
  tree.setTopicDefinition("live:/poses", message);
  QTreeWidgetItem* array = childByText(tree.topicItem("live:/poses"), "poses");

  const std::unique_ptr<QMimeData> unsampled(tree.mimeData({array}));
  tree.setTopicArrayLengths("live:/poses", {{"poses", 2}});
  array = childByText(tree.topicItem("live:/poses"), "poses");
  const std::unique_ptr<QMimeData> sampled(tree.mimeData({array}));

  ASSERT_NE(unsampled, nullptr);
  EXPECT_FALSE(unsampled->hasFormat(rqt_multiplot::kTopicFieldsExpandedMimeType));
  ASSERT_NE(sampled, nullptr);
  ASSERT_TRUE(sampled->hasFormat(rqt_multiplot::kTopicFieldsExpandedMimeType));
  EXPECT_EQ(fields(rqt_multiplot::decodeTopicFields(sampled->data(rqt_multiplot::kTopicFieldsExpandedMimeType))),
            QStringList({"poses/0/x", "poses/0/y", "poses/0/z", "poses/1/x", "poses/1/y", "poses/1/z"}));
}

TEST_F(TopicFieldTreeArrayTest, longArraysAreCappedWithHint) {
  tree_.setTopicArrayLengths("live:/poses", {{"poses", rqt_multiplot::kMaxArrayElementsShown + 5}});

  ASSERT_EQ(posesItem()->childCount(), rqt_multiplot::kMaxArrayElementsShown + 1);
  EXPECT_TRUE(posesItem()->child(rqt_multiplot::kMaxArrayElementsShown)->text(0).contains("5"));
}

class TopicFieldTreeDiagnosticsTest : public ::testing::Test {
 protected:
  static constexpr const char* kKey = "live:/diagnostics";

  void SetUp() override {
    ensureApplication();
    MessageFieldType message;
    message.kind = MessageFieldType::Compound;
    message.members = {{"level", scalar(true)}};
    tree_.addTopic(kKey, "/diagnostics", "diagnostic_msgs/msg/DiagnosticArray", false);
    tree_.setTopicDefinition(kKey, message);
  }

  QTreeWidgetItem* diagnosticsItem() { return childByText(tree_.topicItem(kKey), "Diagnostic values"); }

  TopicFieldTreeWidget tree_;
};

TEST_F(TopicFieldTreeDiagnosticsTest, nodeWaitsForKeysBeforeFirstMessage) {
  ASSERT_NE(diagnosticsItem(), nullptr);
  EXPECT_EQ(tree_.topicItem(kKey)->indexOfChild(diagnosticsItem()), 0);
  ASSERT_EQ(diagnosticsItem()->childCount(), 1);
  EXPECT_EQ(diagnosticsItem()->child(0)->text(0), QString("Waiting for a message..."));
}

TEST_F(TopicFieldTreeDiagnosticsTest, keysAreGroupedBySortedStatusAndHardwareId) {
  tree_.setTopicDiagnosticKeys(kKey,
                               {{"cpu", "host2", "load"}, {"cpu", "host1", "temp"}, {"cpu", "host1", "load"}, {"battery", "", "voltage"}});

  ASSERT_EQ(diagnosticsItem()->childCount(), 3);
  EXPECT_EQ(diagnosticsItem()->child(0)->text(0), QString("battery"));
  EXPECT_EQ(diagnosticsItem()->child(1)->text(0), QString("cpu [host1]"));
  EXPECT_EQ(diagnosticsItem()->child(2)->text(0), QString("cpu [host2]"));
  QTreeWidgetItem* host1 = diagnosticsItem()->child(1);
  ASSERT_EQ(host1->childCount(), 2);
  EXPECT_EQ(host1->child(0)->text(0), QString("load"));
  EXPECT_EQ(host1->child(1)->text(0), QString("temp"));
  EXPECT_TRUE(host1->flags().testFlag(Qt::ItemIsDragEnabled));
  EXPECT_TRUE(host1->child(0)->flags().testFlag(Qt::ItemIsDragEnabled));
}

TEST_F(TopicFieldTreeDiagnosticsTest, keyAndStatusItemsGiveDiagnosticRefs) {
  tree_.setTopicDiagnosticKeys(kKey, {{"cpu", "host1", "load"}, {"cpu", "host1", "temp"}});
  QTreeWidgetItem* status = diagnosticsItem()->child(0);

  const QVector<TopicFieldRef> keyRefs = TopicFieldTreeWidget::refsForItems({status->child(1)});
  const QVector<TopicFieldRef> statusRefs = TopicFieldTreeWidget::refsForItems({status, status->child(0)});

  const QString type = "diagnostic_msgs/msg/DiagnosticArray";
  const QVector<TopicFieldRef> expectedKey = {{"/diagnostics", type, QString(), {"cpu", "host1", "temp"}}};
  const QVector<TopicFieldRef> expectedStatus = {{"/diagnostics", type, QString(), {"cpu", "host1", "load"}},
                                                 {"/diagnostics", type, QString(), {"cpu", "host1", "temp"}}};
  EXPECT_EQ(keyRefs, expectedKey);
  EXPECT_EQ(statusRefs, expectedStatus);
}

TEST_F(TopicFieldTreeDiagnosticsTest, keysSurviveArrayLengthRebuild) {
  tree_.setTopicDiagnosticKeys(kKey, {{"cpu", "", "load"}});
  diagnosticsItem()->child(0)->setExpanded(true);

  tree_.setTopicArrayLengths(kKey, {});

  ASSERT_NE(diagnosticsItem(), nullptr);
  ASSERT_EQ(diagnosticsItem()->childCount(), 1);
  EXPECT_EQ(diagnosticsItem()->child(0)->text(0), QString("cpu"));
  EXPECT_TRUE(diagnosticsItem()->child(0)->isExpanded());
}

TEST_F(TopicFieldTreeDiagnosticsTest, emptyKeySetShowsHint) {
  tree_.setTopicDiagnosticKeys(kKey, {});

  ASSERT_EQ(diagnosticsItem()->childCount(), 1);
  EXPECT_EQ(diagnosticsItem()->child(0)->text(0), QString("No diagnostic values"));
}

TEST(TopicFieldTreeWidget, nonDiagnosticTopicHasNoDiagnosticNode) {
  ensureApplication();
  TopicFieldTreeWidget tree;
  tree.addTopic("live:/pose", "/pose", "test/msg/Pose", false);
  tree.setTopicDefinition("live:/pose", poseMessage());

  EXPECT_EQ(childByText(tree.topicItem("live:/pose"), "Diagnostic values"), nullptr);
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
