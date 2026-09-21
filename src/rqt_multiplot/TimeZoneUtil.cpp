/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TimeZoneUtil.hpp"

#include <QFileInfo>
#include <QRegularExpression>

namespace rqt_multiplot {

namespace {

constexpr auto kHostLocaltimePath = "/run/host/etc/localtime";

bool isUtcTimeZoneId(const QByteArray& id) {
  return (id == "UTC") || (id == "Etc/UTC");
}

QString timeZoneIdFromEnvironment() {
  const QByteArray value = qgetenv("TZ");
  if (value.isEmpty()) {
    return QString();
  }

  QString id = QString::fromUtf8(value).trimmed();
  if (id.startsWith(QLatin1Char(':'))) {
    id = id.mid(1);
  }
  if (id.isEmpty()) {
    return QString();
  }

  const QTimeZone zone(id.toUtf8());
  return zone.isValid() ? id : QString();
}

}  // namespace

QString TimeZoneUtil::timeZoneIdFromLocaltimePath(const QString& localtimePath) {
  const QFileInfo info(localtimePath);
  if (!info.exists()) {
    return QString();
  }

  const QString canonicalPath = info.canonicalFilePath();
  static const QRegularExpression zoneinfoPattern(QStringLiteral("zoneinfo/(.+)$"));
  const QRegularExpressionMatch match = zoneinfoPattern.match(canonicalPath);
  if (!match.hasMatch()) {
    return QString();
  }

  const QString zoneId = match.captured(1);
  const QTimeZone zone(zoneId.toUtf8());
  return zone.isValid() ? zoneId : QString();
}

QString TimeZoneUtil::localTimeZoneId() {
  const QString environmentId = timeZoneIdFromEnvironment();
  if (!environmentId.isEmpty()) {
    return environmentId;
  }

  const QByteArray systemId = QTimeZone::systemTimeZoneId();
  if (!systemId.isEmpty() && !isUtcTimeZoneId(systemId)) {
    return QString::fromUtf8(systemId);
  }

  const QString hostId = timeZoneIdFromLocaltimePath(QString::fromLatin1(kHostLocaltimePath));
  if (!hostId.isEmpty() && !isUtcTimeZoneId(hostId.toUtf8())) {
    return hostId;
  }

  return QString::fromUtf8(systemId);
}

QTimeZone TimeZoneUtil::localTimeZone() {
  const QString id = localTimeZoneId();
  if (id.isEmpty()) {
    return QTimeZone::systemTimeZone();
  }

  const QTimeZone zone(id.toUtf8());
  return zone.isValid() ? zone : QTimeZone::systemTimeZone();
}

}  // namespace rqt_multiplot
