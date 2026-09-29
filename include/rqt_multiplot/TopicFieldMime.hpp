/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <optional>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/TopicMetrics.hpp"

namespace rqt_multiplot {

class CurveConfig;

struct DiagnosticKeyRef {
  QString status;
  QString hardwareId;
  QString key;

  bool operator==(const DiagnosticKeyRef& other) const;
};

struct TopicFieldRef {
  QString topic;
  QString type;
  QString field;
  DiagnosticKeyRef diagnostic;
  std::optional<TopicMetric> metric;

  TopicFieldRef() = default;
  TopicFieldRef(QString topic, QString type, QString field, DiagnosticKeyRef diagnostic = {});

  static TopicFieldRef forMetric(QString topic, QString type, TopicMetric metric);

  bool isDiagnostic() const;
  bool isTopicMetric() const;
  bool operator==(const TopicFieldRef& other) const;
};

extern const QString kTopicFieldsMimeType;
extern const QString kTopicFieldsExpandedMimeType;
// Set only when a single topic root is dragged; holds the topic metric refs offered for that topic.
extern const QString kTopicRootMimeType;
inline constexpr int kMaxCurvesWithoutConfirm = 10;
inline constexpr int kMaxArrayElementsShown = 100;

inline bool requiresDropConfirmation(int curveCount) {
  return curveCount > kMaxCurvesWithoutConfirm;
}

QByteArray encodeTopicFields(const QVector<TopicFieldRef>& refs);
QVector<TopicFieldRef> decodeTopicFields(const QByteArray& data);

bool isArrayWildcardField(const QString& field);
QStringList plottableLeaves(const MessageFieldType& fieldType, const QString& prefix);
// Like plottableLeaves, but also gives the wildcard fields of arrays.
QStringList plottableFields(const MessageFieldType& fieldType, const QString& prefix);
QStringList arrayWildcardFields(const MessageFieldType& arrayType, const QString& path);
bool containsDynamicArray(const MessageFieldType& fieldType);
bool hasHeaderStamp(const MessageFieldType& fieldType);
QVector<TopicFieldRef> topicMetricRefs(const QString& topic, const QString& type, bool withDelay);
void fillCurveFromTopicField(CurveConfig& config, const TopicFieldRef& ref);

QVector<DiagnosticKeyRef> mergeDiagnosticKeys(const QVector<DiagnosticKeyRef>& existing, const QVector<DiagnosticKeyRef>& incoming);

}  // namespace rqt_multiplot
