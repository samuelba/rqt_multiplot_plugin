#include <QBuffer>
#include <QDataStream>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "rqt_multiplot/CurveConfig.hpp"
#include "rqt_multiplot/CurveFilterChainConfig.hpp"
#include "rqt_multiplot/XmlSettings.hpp"

namespace {

using rqt_multiplot::CurveConfig;
using rqt_multiplot::CurveFilterChainConfig;
using rqt_multiplot::CurveFilterComparison;
using rqt_multiplot::CurveFilterSpec;
using rqt_multiplot::CurveFilterType;
using rqt_multiplot::XmlSettings;

QVector<CurveFilterSpec> sampleChain() {
  auto average = rqt_multiplot::defaultCurveFilterSpec(CurveFilterType::MovingAverage);
  average.windowSize = 25;
  average.compensateLag = true;
  auto threshold = rqt_multiplot::defaultCurveFilterSpec(CurveFilterType::Threshold);
  threshold.comparison = CurveFilterComparison::Range;
  threshold.thresholdA = -1.5;
  threshold.thresholdB = 2.5;
  auto derivative = rqt_multiplot::defaultCurveFilterSpec(CurveFilterType::Derivative);
  derivative.useFixedStep = true;
  derivative.fixedStep = 0.02;
  return {average, threshold, derivative, rqt_multiplot::defaultCurveFilterSpec(CurveFilterType::Absolute)};
}

TEST(CurveFilterChainConfig, savesAndLoadsChainInXml) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  const QString path = dir.filePath("filters.xml");

  {
    CurveConfig config;
    config.getFilterChainConfig()->setFilters(sampleChain());
    QSettings settings(path, XmlSettings::format);
    config.save(settings);
    settings.sync();
  }

  CurveConfig loaded;
  QSettings settings(path, XmlSettings::format);
  loaded.load(settings);

  EXPECT_EQ(loaded.getFilterChainConfig()->getFilters(), sampleChain());
}

TEST(CurveFilterChainConfig, oldConfigWithoutFiltersLoadsEmptyChain) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  QSettings settings(dir.filePath("old.ini"), QSettings::IniFormat);
  settings.setValue("title", "Old curve");
  settings.sync();

  CurveConfig config;
  config.getFilterChainConfig()->setFilters(sampleChain());
  config.load(settings);

  EXPECT_TRUE(config.getFilterChainConfig()->isEmpty());
}

TEST(CurveFilterChainConfig, skipsUnknownFilterTypes) {
  QTemporaryDir dir;
  ASSERT_TRUE(dir.isValid());
  QSettings settings(dir.filePath("unknown.ini"), QSettings::IniFormat);
  settings.setValue("filter_0/type", "unknown");
  settings.setValue("filter_1/type", "absolute");
  settings.sync();

  CurveFilterChainConfig config;
  config.load(settings);

  ASSERT_EQ(config.getNumFilters(), 1);
  EXPECT_EQ(config.getFilter(0).type, CurveFilterType::Absolute);
}

TEST(CurveFilterChainConfig, dataStreamRoundTripThroughCurveConfig) {
  CurveConfig source;
  source.setTitle("Filtered");
  source.getFilterChainConfig()->setFilters(sampleChain());

  QByteArray bytes;
  {
    QDataStream out(&bytes, QIODevice::WriteOnly);
    source.write(out);
  }
  CurveConfig copy;
  QDataStream in(bytes);
  copy.read(in);

  EXPECT_EQ(copy.getTitle(), "Filtered");
  EXPECT_EQ(copy.getFilterChainConfig()->getFilters(), sampleChain());
}

TEST(CurveFilterChainConfig, assignmentCopiesChain) {
  CurveConfig source;
  source.getFilterChainConfig()->setFilters(sampleChain());

  CurveConfig copy;
  copy = source;

  EXPECT_EQ(copy.getFilterChainConfig()->getFilters(), sampleChain());
}

TEST(CurveFilterChainConfig, editsEmitChangedAndKeepOrder) {
  CurveConfig curve;
  CurveFilterChainConfig* chain = curve.getFilterChainConfig();
  QSignalSpy curveChanged(&curve, SIGNAL(changed()));

  chain->addFilter(rqt_multiplot::defaultCurveFilterSpec(CurveFilterType::Derivative));
  chain->addFilter(rqt_multiplot::defaultCurveFilterSpec(CurveFilterType::Integral));
  chain->moveFilter(1, 0);
  chain->removeFilter(1);
  chain->removeFilter(5);

  ASSERT_EQ(chain->getNumFilters(), 1);
  EXPECT_EQ(chain->getFilter(0).type, CurveFilterType::Integral);
  EXPECT_EQ(curveChanged.count(), 4);
}

}  // namespace
