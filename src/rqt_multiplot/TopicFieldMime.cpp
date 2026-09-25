/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#include "rqt_multiplot/TopicFieldMime.hpp"

#include <algorithm>

#include <QDataStream>
#include <QIODevice>

#include "rqt_multiplot/CurveConfig.hpp"

namespace rqt_multiplot {

namespace {

constexpr quint32 kPayloadMagic = 0x54464d31;  // "TFM1"

QString joinPath(const QString& prefix, const QString& name) {
  return prefix.isEmpty() ? name : prefix + "/" + name;
}

bool isArrayWildcard(const QString& field) {
  return field.split('/').contains(QStringLiteral("*"));
}

}  // namespace

const QString kTopicFieldsMimeType = QStringLiteral("application/rqt-multiplot-topic-fields");

bool TopicFieldRef::operator==(const TopicFieldRef& other) const {
  return (topic == other.topic) && (type == other.type) && (field == other.field);
}

QByteArray encodeTopicFields(const QVector<TopicFieldRef>& refs) {
  QByteArray data;
  QDataStream stream(&data, QIODevice::WriteOnly);
  stream << kPayloadMagic << static_cast<quint32>(refs.count());
  for (const auto& ref : refs) {
    stream << ref.topic << ref.type << ref.field;
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
    stream >> ref.topic >> ref.type >> ref.field;
    if (stream.status() != QDataStream::Ok) {
      return {};
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

void fillCurveFromTopicField(CurveConfig& config, const TopicFieldRef& ref) {
  CurveAxisConfig* x = config.getAxisConfig(CurveConfig::X);
  x->setTopic(ref.topic);
  x->setType(ref.type);
  x->setFieldType(isArrayWildcard(ref.field) ? CurveAxisConfig::ArrayIndex : CurveAxisConfig::MessageReceiptTime);

  CurveAxisConfig* y = config.getAxisConfig(CurveConfig::Y);
  y->setTopic(ref.topic);
  y->setType(ref.type);
  y->setFieldType(CurveAxisConfig::MessageData);
  y->setField(ref.field);

  config.setTitle(joinPath(ref.topic, ref.field));
}

}  // namespace rqt_multiplot
