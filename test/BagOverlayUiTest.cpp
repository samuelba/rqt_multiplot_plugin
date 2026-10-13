#include <QAction>
#include <QDoubleSpinBox>
#include <QStringList>

#include <gtest/gtest.h>

#include "OverlayTestBags.hpp"
#include "rqt_multiplot/BagOverlayDialog.hpp"
#include "rqt_multiplot/BagOverlaySession.hpp"
#include "rqt_multiplot/MultiplotConfig.hpp"
#include "rqt_multiplot/PlotConfig.hpp"
#include "rqt_multiplot/PlotTabWidget.hpp"
#include "rqt_multiplot/PlotTableConfigWidget.hpp"
#include "rqt_multiplot/PlotTableWidget.hpp"
#include "rqt_multiplot/PlotWidget.hpp"
#include "rqt_multiplot/Theme.hpp"

namespace {

using rqt_multiplot::BagOverlayDialog;
using rqt_multiplot::BagOverlaySession;
using rqt_multiplot::MultiplotConfig;
using rqt_multiplot::PlotTableConfigWidget;
using rqt_multiplot::PlotTabWidget;

class BagOverlayUiTest : public ::testing::Test {
 protected:
  void SetUp() override {
    rqt_multiplot::test::ensureOverlayTestApplication();
    tabs_.setConfig(&config_);
    tabs_.getPlotTable(0)->getPlotWidgets().front()->getConfig()->addCurve();
    toolbar_.setPlotTabs(&tabs_);
    paths_ =
        QStringList{bags_.writeFloatBag("run1", 100, 1.0), bags_.writeFloatBag("run2", 105, 2.0), bags_.writeFloatBag("run3", 107, 3.0)};
  }

  rqt_multiplot::test::TempBagDirectory bags_;
  MultiplotConfig config_{nullptr};
  PlotTabWidget tabs_;
  PlotTableConfigWidget toolbar_;
  QStringList paths_;
};

TEST_F(BagOverlayUiTest, offsetsActionIsEnabledOnlyWhileOverlayIsLoaded) {
  EXPECT_FALSE(toolbar_.getActionOverlayOffsets()->isEnabled());

  ASSERT_TRUE(toolbar_.loadOverlayBagPaths(paths_.mid(0, 2)));
  EXPECT_TRUE(toolbar_.getActionOverlayOffsets()->isEnabled());

  tabs_.loadFromBagFiles(QStringList{paths_[0]}, true);
  EXPECT_FALSE(toolbar_.getActionOverlayOffsets()->isEnabled());
}

TEST_F(BagOverlayUiTest, rejectsSingleBag) {
  EXPECT_FALSE(toolbar_.loadOverlayBagPaths(paths_.mid(0, 1)));
  EXPECT_FALSE(toolbar_.getActionOverlayOffsets()->isEnabled());
  EXPECT_FALSE(toolbar_.getOverlaySession()->getError().isEmpty());
}

TEST_F(BagOverlayUiTest, dialogShowsOneReadOnlyReferenceAndEditableOffsets) {
  ASSERT_TRUE(toolbar_.loadOverlayBagPaths(paths_));

  toolbar_.getActionOverlayOffsets()->trigger();
  BagOverlayDialog* dialog = toolbar_.getOverlayDialog();
  ASSERT_NE(dialog, nullptr);

  ASSERT_NE(dialog->getOffsetSpinBox(2), nullptr);
  EXPECT_EQ(dialog->getOffsetSpinBox(3), nullptr);
  EXPECT_TRUE(dialog->getOffsetSpinBox(0)->isReadOnly());
  EXPECT_FALSE(dialog->getOffsetSpinBox(1)->isReadOnly());
  EXPECT_DOUBLE_EQ(dialog->getOffsetSpinBox(1)->value(), -5.0);
  EXPECT_DOUBLE_EQ(dialog->getOffsetSpinBox(2)->value(), -7.0);
}

TEST_F(BagOverlayUiTest, dialogUsesCurrentTheme) {
  ASSERT_TRUE(toolbar_.loadOverlayBagPaths(paths_.mid(0, 2)));
  rqt_multiplot::Theme::apply(nullptr, rqt_multiplot::Theme::Id::Dark);

  toolbar_.getActionOverlayOffsets()->trigger();
  const QColor window = toolbar_.getOverlayDialog()->palette().color(QPalette::Window);
  rqt_multiplot::Theme::apply(nullptr, rqt_multiplot::Theme::Id::Light);

  EXPECT_EQ(window, rqt_multiplot::Theme::palette(rqt_multiplot::Theme::Id::Dark).color(QPalette::Window));
}

TEST_F(BagOverlayUiTest, dialogAndSessionOffsetsStayInSync) {
  ASSERT_TRUE(toolbar_.loadOverlayBagPaths(paths_));
  toolbar_.getActionOverlayOffsets()->trigger();
  BagOverlayDialog* dialog = toolbar_.getOverlayDialog();
  BagOverlaySession* session = toolbar_.getOverlaySession();

  dialog->getOffsetSpinBox(1)->setValue(-4.5);
  EXPECT_DOUBLE_EQ(session->getOffset(1), -4.5);

  session->setOffset(2, -6.25);
  EXPECT_DOUBLE_EQ(dialog->getOffsetSpinBox(2)->value(), -6.25);

  session->clear();
  EXPECT_EQ(dialog->getOffsetSpinBox(0), nullptr);
}

}  // namespace
