#include <cstdlib>

#include <QFileInfo>

#include <gtest/gtest.h>

#include <rqt_multiplot/TimeZoneUtil.h>

namespace {

using rqt_multiplot::TimeZoneUtil;

class ScopedTzEnvironment {
 public:
  explicit ScopedTzEnvironment(const QByteArray& value) : hadPrevious_(!qgetenv("TZ").isEmpty()) {
    if (hadPrevious_) {
      previous_ = qgetenv("TZ");
    }
    if (value.isEmpty()) {
      qunsetenv("TZ");
    } else {
      qputenv("TZ", value);
    }
  }

  ~ScopedTzEnvironment() {
    if (hadPrevious_) {
      qputenv("TZ", previous_);
    } else {
      qunsetenv("TZ");
    }
  }

 private:
  bool hadPrevious_;
  QByteArray previous_;
};

TEST(TimeZoneUtil, timeZoneIdFromLocaltimePathReadsZoneinfoSymlink) {
  const QFileInfo hostLocaltime(QStringLiteral("/run/host/etc/localtime"));
  if (!hostLocaltime.exists()) {
    GTEST_SKIP() << "/run/host/etc/localtime unavailable";
  }

  const QString zoneId = TimeZoneUtil::timeZoneIdFromLocaltimePath(hostLocaltime.absoluteFilePath());
  ASSERT_FALSE(zoneId.isEmpty());

  const QTimeZone zone(zoneId.toUtf8());
  EXPECT_TRUE(zone.isValid());
}

TEST(TimeZoneUtil, localTimeZonePrefersTzEnvironment) {
  const QTimeZone berlin(QStringLiteral("Europe/Berlin").toUtf8());
  if (!berlin.isValid()) {
    GTEST_SKIP() << "Europe/Berlin unavailable in Qt tzdata";
  }

  ScopedTzEnvironment tz(QByteArray("Europe/Berlin"));
  EXPECT_EQ(TimeZoneUtil::localTimeZoneId(), QStringLiteral("Europe/Berlin"));
  EXPECT_EQ(TimeZoneUtil::localTimeZone(), berlin);
}

TEST(TimeZoneUtil, localTimeZoneUsesHostWhenContainerReportsUtc) {
  ScopedTzEnvironment tz{QByteArray()};
  const QFileInfo hostLocaltime(QStringLiteral("/run/host/etc/localtime"));
  if (!hostLocaltime.exists()) {
    GTEST_SKIP() << "/run/host/etc/localtime unavailable";
  }

  const QString hostId = TimeZoneUtil::timeZoneIdFromLocaltimePath(hostLocaltime.absoluteFilePath());
  if (hostId.isEmpty() || hostId == QStringLiteral("UTC") || hostId == QStringLiteral("Etc/UTC")) {
    GTEST_SKIP() << "host timezone is UTC or unavailable";
  }

  if (QTimeZone::systemTimeZoneId() != QByteArray("UTC") && QTimeZone::systemTimeZoneId() != QByteArray("Etc/UTC")) {
    GTEST_SKIP() << "system timezone is not UTC";
  }

  EXPECT_EQ(TimeZoneUtil::localTimeZoneId(), hostId);
  EXPECT_EQ(TimeZoneUtil::localTimeZone(), QTimeZone(hostId.toUtf8()));
}

}  // namespace
