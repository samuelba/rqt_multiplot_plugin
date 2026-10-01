#include <memory>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMimeData>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveAxisConfig.hpp"
#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveData.hpp"
#include "rqt_multiplot/CurveFilterChainWidget.hpp"
#include "rqt_multiplot/CurveFilterDropDialog.hpp"
#include "rqt_multiplot/CurveFilterPanelWidget.hpp"
#include "rqt_multiplot/CurveFilterParamsWidget.hpp"
#include "rqt_multiplot/CurveStyleConfig.hpp"
#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/MultiplotWidget.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotCurve.hpp"
#include "rqt_multiplot/PlotTableConfig.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"

namespace {

using rqt_multiplot::CurveAxisConfig;
using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveFilterChainConfig;
using rqt_multiplot::CurveFilterChainWidget;
using rqt_multiplot::CurveFilterDropDialog;
using rqt_multiplot::CurveFilterPanelWidget;
using rqt_multiplot::CurveFilterParamsWidget;
using rqt_multiplot::CurveFilterSpec;
using rqt_multiplot::CurveFilterType;
using rqt_multiplot::defaultCurveFilterSpec;
using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::MultiplotWidget;
using rqt_multiplot::PlotConfig;
using rqt_multiplot::PlotCurve;
using rqt_multiplot::PlotTableConfig;
using rqt_multiplot::PlotTableWidget;
using rqt_multiplot::PlotWidget;

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

CurveFilterSpec movingAverage(int windowSize) {
  CurveFilterSpec spec = defaultCurveFilterSpec(CurveFilterType::MovingAverage);
  spec.windowSize = windowSize;
  return spec;
}

QVector<CurveFilterType> filterTypes(const CurveFilterChainConfig& config) {
  QVector<CurveFilterType> types;
  for (const CurveFilterSpec& spec : config.getFilters()) {
    types.push_back(spec.type);
  }
  return types;
}

void configureSnapshotCurve(CurveConfig* config) {
  config->getAxisConfig(CurveConfig::X)->setTopic("/array");
  config->getAxisConfig(CurveConfig::Y)->setTopic("/array");
  config->getAxisConfig(CurveConfig::X)->setFieldType(CurveAxisConfig::ArrayIndex);
  config->getAxisConfig(CurveConfig::Y)->setField("position/*");
  config->getStyleConfig()->setFadeHistory(3);
}

void addCurves(PlotConfig& plot) {
  plot.addCurve()->setTitle("Pan");
  plot.addCurve()->setTitle("Tilt");
  CurveConfig* snapshot = plot.addCurve();
  snapshot->setTitle("Array");
  configureSnapshotCurve(snapshot);
}

bool sendDrop(PlotWidget& widget, const QMimeData* mimeData) {
  QDragEnterEvent enter(widget.rect().center(), Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(&widget, &enter);
  if (!enter.isAccepted()) {
    return false;
  }
  QDropEvent drop(QPointF(widget.rect().center()), Qt::CopyAction, mimeData, Qt::LeftButton, Qt::NoModifier);
  QApplication::sendEvent(&widget, &drop);
  return drop.isAccepted();
}

void answerDropDialog(QDialogButtonBox::StandardButton button, int windowSize = 0) {
  QTimer::singleShot(0, [button, windowSize]() {
    auto* dialog = qobject_cast<CurveFilterDropDialog*>(QApplication::activeModalWidget());
    ASSERT_NE(dialog, nullptr);
    if (windowSize > 0) {
      auto* spinBox = dialog->findChild<QSpinBox*>(QStringLiteral("windowSizeSpinBox"));
      ASSERT_NE(spinBox, nullptr);
      spinBox->setValue(windowSize);
    }
    dialog->findChild<QDialogButtonBox*>(QStringLiteral("curveFilterDropButtonBox"))->button(button)->click();
  });
}

class CurveFilterWidgets : public ::testing::Test {
 protected:
  void SetUp() override {
    ensureApplication();
    CurveFilterDropDialog::forgetSpecs();
  }
};

TEST_F(CurveFilterWidgets, paramsWidgetEditsEmitSpecChanged) {
  CurveFilterParamsWidget widget;
  widget.setSpec(movingAverage(4));
  QSignalSpy spy(&widget, &CurveFilterParamsWidget::specChanged);

  auto* spinBox = widget.findChild<QSpinBox*>(QStringLiteral("windowSizeSpinBox"));
  ASSERT_NE(spinBox, nullptr);
  EXPECT_EQ(spinBox->value(), 4);
  spinBox->setValue(9);

  EXPECT_EQ(spy.count(), 1);
  EXPECT_EQ(widget.getSpec().windowSize, 9);
  EXPECT_EQ(widget.findChild<QCheckBox*>(QStringLiteral("standardDeviationCheckBox")), nullptr);
}

TEST_F(CurveFilterWidgets, paramsWidgetShowsNoParametersForAbsolute) {
  CurveFilterParamsWidget widget;
  widget.setSpec(defaultCurveFilterSpec(CurveFilterType::Absolute));

  EXPECT_NE(widget.findChild<QLabel*>(QStringLiteral("noParametersLabel")), nullptr);
  EXPECT_EQ(widget.findChild<QSpinBox*>(QStringLiteral("windowSizeSpinBox")), nullptr);
  EXPECT_EQ(widget.findChild<QLabel*>(QStringLiteral("curveFilterDescriptionLabel"))->text(),
            rqt_multiplot::curveFilterTypeDescription(CurveFilterType::Absolute));
}

TEST_F(CurveFilterWidgets, chainWidgetReordersAndRemovesFilters) {
  CurveFilterChainConfig config;
  config.setFilters(
      {defaultCurveFilterSpec(CurveFilterType::Derivative), defaultCurveFilterSpec(CurveFilterType::Absolute), movingAverage(3)});
  CurveFilterChainWidget widget;
  widget.setChainConfig(&config);

  widget.setCurrentFilterIndex(2);
  widget.findChild<QToolButton*>(QStringLiteral("curveFilterMoveUpButton"))->click();
  EXPECT_EQ(filterTypes(config),
            QVector<CurveFilterType>({CurveFilterType::Derivative, CurveFilterType::MovingAverage, CurveFilterType::Absolute}));
  EXPECT_EQ(widget.getCurrentFilterIndex(), 1);

  widget.findChild<QToolButton*>(QStringLiteral("curveFilterRemoveButton"))->click();
  EXPECT_EQ(filterTypes(config), QVector<CurveFilterType>({CurveFilterType::Derivative, CurveFilterType::Absolute}));
}

TEST_F(CurveFilterWidgets, chainWidgetAddsFromMenuAndEditsLive) {
  CurveFilterChainConfig config;
  CurveFilterChainWidget widget;
  widget.setChainConfig(&config);

  QMenu* menu = widget.findChild<QToolButton*>(QStringLiteral("curveFilterAddButton"))->menu();
  ASSERT_NE(menu, nullptr);
  for (QAction* action : menu->actions()) {
    if (action->data().toString() == QStringLiteral("moving_average")) {
      action->trigger();
    }
  }
  ASSERT_EQ(config.getNumFilters(), 1);
  EXPECT_EQ(widget.getCurrentFilterIndex(), 0);

  auto* spinBox = widget.findChild<QSpinBox*>(QStringLiteral("windowSizeSpinBox"));
  ASSERT_NE(spinBox, nullptr);
  spinBox->setValue(25);
  EXPECT_EQ(config.getFilter(0).windowSize, 25);
  EXPECT_EQ(widget.findChild<QSpinBox*>(QStringLiteral("windowSizeSpinBox")), spinBox);
}

TEST_F(CurveFilterWidgets, dropDialogIsSkippedForParameterlessFilters) {
  PlotConfig plot;
  addCurves(plot);

  const auto result = CurveFilterDropDialog::ask(nullptr, CurveFilterType::Absolute, plot);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->spec.type, CurveFilterType::Absolute);
  EXPECT_EQ(result->curveIndices, QVector<int>({0, 1}));
}

TEST_F(CurveFilterWidgets, dropDialogPrefillsLastUsedSpecAndDisablesSnapshots) {
  PlotConfig plot;
  addCurves(plot);
  CurveFilterDropDialog::rememberSpec(movingAverage(7));

  CurveFilterDropDialog dialog(nullptr, CurveFilterType::MovingAverage, plot);

  EXPECT_EQ(dialog.findChild<QSpinBox*>(QStringLiteral("windowSizeSpinBox"))->value(), 7);
  auto* list = dialog.findChild<QListWidget*>(QStringLiteral("curveFilterDropCurveList"));
  ASSERT_EQ(list->count(), 3);
  EXPECT_EQ(list->item(2)->checkState(), Qt::Unchecked);
  EXPECT_FALSE(list->item(2)->flags().testFlag(Qt::ItemIsEnabled));
  EXPECT_EQ(dialog.getResult().curveIndices, QVector<int>({0, 1}));
}

TEST_F(CurveFilterWidgets, plotDropAddsParameterlessFilterToFilterableCurves) {
  PlotConfig config;
  addCurves(config);
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(rqt_multiplot::createCurveFilterMimeData(CurveFilterType::Absolute));

  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  EXPECT_EQ(config.getCurveConfig(0)->getFilterChainConfig()->getNumFilters(), 1);
  EXPECT_EQ(config.getCurveConfig(1)->getFilterChainConfig()->getNumFilters(), 1);
  EXPECT_TRUE(config.getCurveConfig(2)->getFilterChainConfig()->isEmpty());
}

TEST_F(CurveFilterWidgets, plotDropCancelAddsNothing) {
  PlotConfig config;
  addCurves(config);
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(rqt_multiplot::createCurveFilterMimeData(CurveFilterType::MovingAverage));

  answerDropDialog(QDialogButtonBox::Cancel);
  EXPECT_FALSE(sendDrop(widget, mimeData.get()));

  EXPECT_TRUE(config.getCurveConfig(0)->getFilterChainConfig()->isEmpty());
}

TEST_F(CurveFilterWidgets, plotDropOkAddsEditedSpecAndRemembersIt) {
  PlotConfig config;
  addCurves(config);
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(rqt_multiplot::createCurveFilterMimeData(CurveFilterType::MovingAverage));

  answerDropDialog(QDialogButtonBox::Ok, 12);
  EXPECT_TRUE(sendDrop(widget, mimeData.get()));

  EXPECT_EQ(config.getCurveConfig(0)->getFilterChainConfig()->getFilter(0), movingAverage(12));
  EXPECT_EQ(config.getCurveConfig(1)->getFilterChainConfig()->getFilter(0), movingAverage(12));
  EXPECT_EQ(CurveFilterDropDialog::lastUsedSpec(CurveFilterType::MovingAverage).windowSize, 12);
}

TEST_F(CurveFilterWidgets, plotRejectsFilterDropWithoutCurves) {
  PlotConfig config;
  PlotWidget widget;
  widget.setConfig(&config);
  const std::unique_ptr<QMimeData> mimeData(rqt_multiplot::createCurveFilterMimeData(CurveFilterType::Absolute));

  EXPECT_FALSE(sendDrop(widget, mimeData.get()));
}

TEST_F(CurveFilterWidgets, panelEditsSelectedCurveAndAddsFilteredCopy) {
  PlotTableConfig tableConfig(nullptr);
  PlotConfig* plot = tableConfig.getPlotConfig(0, 0);
  addCurves(*plot);
  PlotTableWidget table;
  table.setConfig(&tableConfig);
  CurveFilterPanelWidget panel;
  panel.setPlotTable(&table);

  auto* tree = panel.findChild<QTreeWidget*>(QStringLiteral("curveFilterCurveTree"));
  ASSERT_EQ(tree->topLevelItemCount(), 1);
  EXPECT_EQ(tree->topLevelItem(0)->childCount(), 3);
  EXPECT_FALSE(tree->topLevelItem(0)->child(2)->flags().testFlag(Qt::ItemIsEnabled));

  ASSERT_EQ(table.getPlotWidgets().count(), 1);
  PlotCurve* source = table.getPlotWidgets().first()->getCurves().first();
  for (const QPointF& point : {QPointF(0.0, 1.0), QPointF(1.0, 3.0), QPointF(2.0, 5.0)}) {
    source->getData()->appendPoint(point);
  }

  panel.selectPlot(plot);
  ASSERT_EQ(panel.getCurrentCurve(), plot->getCurveConfig(0));
  EXPECT_EQ(panel.findChild<QLabel*>(QStringLiteral("curveFilterChainHeading"))->text(), QStringLiteral("Filter chain of \"Pan\""));
  auto* palette = panel.findChild<QListWidget*>(QStringLiteral("curveFilterPalette"));
  emit palette->itemDoubleClicked(palette->item(0));
  EXPECT_EQ(plot->getCurveConfig(0)->getFilterChainConfig()->getNumFilters(), 1);

  panel.findChild<QPushButton*>(QStringLiteral("curveFilterCopyButton"))->click();
  ASSERT_EQ(plot->getNumCurves(), 4u);
  CurveConfig* copy = plot->getCurveConfig(3);
  EXPECT_EQ(copy->getTitle(), QStringLiteral("Pan [filtered]"));
  EXPECT_EQ(copy->getFilterChainConfig()->getNumFilters(), 1);
  EXPECT_EQ(panel.getCurrentCurve(), copy);

  const PlotCurve* copyCurve = table.getPlotWidgets().first()->getCurves().at(3);
  ASSERT_EQ(copyCurve->getConfig(), copy);
  EXPECT_EQ(copyCurve->getRawData()->getNumPoints(), 3u);
  ASSERT_EQ(copyCurve->getData()->getNumPoints(), 2u);
  EXPECT_DOUBLE_EQ(copyCurve->getData()->getPoint(1).y(), 2.0);
}

TEST_F(CurveFilterWidgets, sideRailPanelsAreExclusive) {
  MultiplotWidget widget;
  auto* topicButton = widget.findChild<QPushButton*>(QStringLiteral("pushButtonTopicBrowser"));
  auto* filterButton = widget.findChild<QPushButton*>(QStringLiteral("pushButtonCurveFilters"));
  auto* panel = widget.findChild<CurveFilterPanelWidget*>();
  ASSERT_NE(topicButton, nullptr);
  ASSERT_NE(filterButton, nullptr);
  ASSERT_NE(panel, nullptr);
  widget.show();

  filterButton->click();
  EXPECT_EQ(widget.getConfig()->getSidePanel(), MultiplotConfig::SidePanel::CurveFilters);
  EXPECT_TRUE(filterButton->isChecked());
  EXPECT_FALSE(topicButton->isChecked());
  EXPECT_TRUE(panel->isVisible());

  topicButton->click();
  EXPECT_EQ(widget.getConfig()->getSidePanel(), MultiplotConfig::SidePanel::TopicBrowser);
  EXPECT_FALSE(filterButton->isChecked());
  EXPECT_FALSE(panel->isVisible());

  topicButton->click();
  EXPECT_EQ(widget.getConfig()->getSidePanel(), MultiplotConfig::SidePanel::None);
  EXPECT_FALSE(topicButton->isChecked());
}

}  // namespace
