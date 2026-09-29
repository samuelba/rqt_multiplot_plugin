/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TopicFieldMime.hpp"

#include <algorithm>
#include <utility>

#include <QDataStream>
#include <QIODevice>

#include "rqt_multiplot/CurveConfig.hpp"

namespace rqt_multiplot {

namespace {

constexpr quint32 kPayloadMagic = 0x54464d33;  // "TFM3"
constexpr qint32 kNoMetric = -1;

QString joinPath(const QString& prefix, const QString& name) {
  return prefix.isEmpty() ? name : prefix + "/" + name;
}

}  // namespace

bool isArrayWildcardField(const QString& field) {
  return field.split('/').contains(QStringLiteral("*"));
}

const QString kTopicFieldsMimeType = QStringLiteral("application/rqt-multiplot-topic-fields");
const QString kTopicFieldsExpandedMimeType = QStringLiteral("application/rqt-multiplot-topic-fields-expanded");
const QString kTopicRootMimeType = QStringLiteral("application/rqt-multiplot-topic-root");

bool DiagnosticKeyRef::operator==(const DiagnosticKeyRef& other) const {
  return (status == other.status) && (hardwareId == other.hardwareId) && (key == other.key);
}

TopicFieldRef::TopicFieldRef(QString topic, QString type, QString field, DiagnosticKeyRef diagnostic)
    : topic(std::move(topic)), type(std::move(type)), field(std::move(field)), diagnostic(std::move(diagnostic)) {}

TopicFieldRef TopicFieldRef::forMetric(QString topic, QString type, TopicMetric metric) {
  TopicFieldRef ref(std::move(topic), std::move(type), QString());
  ref.metric = metric;
  return ref;
}

bool TopicFieldRef::isDiagnostic() const {
  return !diagnostic.status.isEmpty() && !diagnostic.key.isEmpty();
}

bool TopicFieldRef::isTopicMetric() const {
  return metric.has_value();
}

bool TopicFieldRef::operator==(const TopicFieldRef& other) const {
  return (topic == other.topic) && (type == other.type) && (field == other.field) && (diagnostic == other.diagnostic) &&
         (metric == other.metric);
}

QByteArray encodeTopicFields(const QVector<TopicFieldRef>& refs) {
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << kPayloadMagic << static_cast<quint32>(refs.count());
  for (const auto& ref : refs) {
    stream << ref.topic << ref.type << ref.field << ref.diagnostic.status << ref.diagnostic.hardwareId << ref.diagnostic.key
           << (ref.metric ? static_cast<qint32>(*ref.metric) : kNoMetric);
  }
  return data;
}

QVector<TopicFieldRef> decodeTopicFields(const QByteArray& data) {
  QDataStream stream(data);
  quint32 magic = 0;
  quint32 count = 0;
  stream >> magic >> count;
  if ((stream.status() != QDataStream::Ok) || (magic != kPayloadMagic)) {
    return {};
  }

  QVector<TopicFieldRef> refs;
  for (quint32 i = 0; i < count; ++i) {
    TopicFieldRef ref;
    qint32 metric = kNoMetric;
    stream >> ref.topic >> ref.type >> ref.field >> ref.diagnostic.status >> ref.diagnostic.hardwareId >> ref.diagnostic.key >> metric;
    if ((stream.status() != QDataStream::Ok) || (metric < kNoMetric) || (metric >= kTopicMetricCount)) {
      return {};
    }
    if (metric != kNoMetric) {
      ref.metric = static_cast<TopicMetric>(metric);
    }
    refs.append(ref);
  }
  return refs;
}

QStringList plottableLeaves(const MessageFieldType& fieldType, const QString& prefix) {
  if (fieldType.isNumeric) {
    return prefix.isEmpty() ? QStringList() : QStringList(prefix);
  }
  if (!fieldType.isMessage()) {
    return {};
  }

  QStringList leaves;
  for (const auto& member : fieldType.members) {
    leaves.append(plottableLeaves(member.second, joinPath(prefix, member.first)));
  }
  return leaves;
}

QStringList plottableFields(const MessageFieldType& fieldType, const QString& prefix) {
  if (fieldType.isNumeric) {
    return prefix.isEmpty() ? QStringList() : QStringList(prefix);
  }
  if (fieldType.isArray()) {
    return arrayWildcardFields(fieldType, prefix);
  }
  if (!fieldType.isMessage()) {
    return {};
  }

  QStringList fields;
  for (const auto& member : fieldType.members) {
    fields.append(plottableFields(member.second, joinPath(prefix, member.first)));
  }
  return fields;
}

QStringList arrayWildcardFields(const MessageFieldType& arrayType, const QString& path) {
  if (!arrayType.isArray() || !arrayType.elementType) {
    return {};
  }
  const QString wildcardPath = joinPath(path, QStringLiteral("*"));
  if (arrayType.elementType->isNumeric) {
    return {wildcardPath};
  }
  return plottableLeaves(*arrayType.elementType, wildcardPath);
}

bool containsDynamicArray(const MessageFieldType& fieldType) {
  if (fieldType.isArray()) {
    return fieldType.isDynamicArray || (fieldType.elementType && containsDynamicArray(*fieldType.elementType));
  }
  return std::any_of(fieldType.members.cbegin(), fieldType.members.cend(),
                     [](const QPair<QString, MessageFieldType>& member) { return containsDynamicArray(member.second); });
}

bool hasHeaderStamp(const MessageFieldType& fieldType) {
  for (const auto& member : fieldType.members) {
    if (member.first == QLatin1String("header")) {
      return std::any_of(member.second.members.cbegin(), member.second.members.cend(),
                         [](const QPair<QString, MessageFieldType>& headerMember) { return headerMember.first == QLatin1String("stamp"); });
    }
  }
  return false;
}

QVector<TopicFieldRef> topicMetricRefs(const QString& topic, const QString& type, bool withDelay) {
  QVector<TopicFieldRef> refs;
  for (int index = 0; index < kTopicMetricCount; ++index) {
    const auto metric = static_cast<TopicMetric>(index);
    if (withDelay || !isDelayMetric(metric)) {
      refs.append(TopicFieldRef::forMetric(topic, type, metric));
    }
  }
  return refs;
}

void fillCurveFromTopicField(CurveConfig& config, const TopicFieldRef& ref) {
  CurveAxisConfig* x = config.getAxisConfig(CurveConfig::X);
  x->setTopic(ref.topic);
  x->setType(ref.type);
  x->setFieldType(isArrayWildcardField(ref.field) ? CurveAxisConfig::ArrayIndex : CurveAxisConfig::MessageReceiptTime);

  CurveAxisConfig* y = config.getAxisConfig(CurveConfig::Y);
  y->setTopic(ref.topic);
  y->setType(ref.type);

  if (ref.metric) {
    y->setFieldType(CurveAxisConfig::TopicMetric);
    y->setTopicMetric(*ref.metric);
    config.setTitle(joinPath(ref.topic, QString::fromUtf8(topicMetricName(*ref.metric))));
    return;
  }

  if (ref.isDiagnostic()) {
    y->setFieldType(CurveAxisConfig::DiagnosticValue);
    y->setDiagnosticStatus(ref.diagnostic.status);
    y->setDiagnosticHardwareId(ref.diagnostic.hardwareId);
    y->setDiagnosticKey(ref.diagnostic.key);
    const QString title = joinPath(joinPath(ref.topic, ref.diagnostic.status), ref.diagnostic.key);
    config.setTitle(ref.diagnostic.hardwareId.isEmpty() ? title : title + QStringLiteral(" [") + ref.diagnostic.hardwareId + ']');
    return;
  }

  y->setFieldType(CurveAxisConfig::MessageData);
  y->setField(ref.field);
  config.setTitle(joinPath(ref.topic, ref.field));
}

QVector<DiagnosticKeyRef> mergeDiagnosticKeys(const QVector<DiagnosticKeyRef>& existing, const QVector<DiagnosticKeyRef>& incoming) {
  QVector<DiagnosticKeyRef> merged = existing;
  for (const auto& key : incoming) {
    if (!merged.contains(key)) {
      merged.append(key);
    }
  }
  return merged;
}

}  // namespace rqt_multiplot
