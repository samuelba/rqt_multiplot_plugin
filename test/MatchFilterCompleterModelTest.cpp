#include <gtest/gtest.h>

#include <QStringListModel>

#include <rqt_multiplot/MatchFilterCompleterModel.h>

namespace {

TEST(MatchFilterCompleterModel, setFilterKeyKeepsMatchingRows) {
  QStringListModel source({"alpha", "beta", "alpine"});
  rqt_multiplot::MatchFilterCompleterModel proxy;
  proxy.setSourceModel(&source);
  proxy.setFilterMatchFlags(Qt::MatchStartsWith);
  proxy.setFilterKey("al");

  EXPECT_EQ(proxy.rowCount(), 2);
}

}  // namespace
