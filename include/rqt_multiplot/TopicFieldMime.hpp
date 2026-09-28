/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVector>

#include "rqt_multiplot/MessageFieldType.hpp"

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

  TopicFieldRef() = default;
  TopicFieldRef(QString topic, QString type, QString field, DiagnosticKeyRef diagnostic = {});

  bool isDiagnostic() const;
  bool operator==(const TopicFieldRef& other) const;
};

extern const QString kTopicFieldsMimeType;
extern const QString kTopicFieldsExpandedMimeType;
inline constexpr int kMaxCurvesWithoutConfirm = 10;
inline constexpr int kMaxArrayElementsShown = 100;

inline bool requiresDropConfirmation(int curveCount) {
  return curveCount > kMaxCurvesWithoutConfirm;
}

QByteArray encodeTopicFields(const QVector<TopicFieldRef>& refs);
QVector<TopicFieldRef> decodeTopicFields(const QByteArray& data);

QStringList plottableLeaves(const MessageFieldType& fieldType, const QString& prefix);
QStringList arrayWildcardFields(const MessageFieldType& arrayType, const QString& path);
bool containsDynamicArray(const MessageFieldType& fieldType);
void fillCurveFromTopicField(CurveConfig& config, const TopicFieldRef& ref);

QVector<DiagnosticKeyRef> mergeDiagnosticKeys(const QVector<DiagnosticKeyRef>& existing, const QVector<DiagnosticKeyRef>& incoming);

}  // namespace rqt_multiplot
