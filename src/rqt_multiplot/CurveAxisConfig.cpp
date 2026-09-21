/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 *                                                                            *
 * This program is free software; you can redistribute it and/or modify       *
 * it under the terms of the Lesser GNU General Public License as published by*
 * the Free Software Foundation; either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the               *
 * Lesser GNU General Public License for more details.                        *
 *                                                                            *
 * You should have received a copy of the Lesser GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.       *
 ******************************************************************************/

#include "rqt_multiplot/CurveAxisConfig.hpp"

#include <QtMath>

#include <utility>

namespace rqt_multiplot {
namespace {

CurveAxisConfig::FieldType parseFieldType(int raw) {
  switch (raw) {
    case CurveAxisConfig::MessageData:
    case CurveAxisConfig::MessageReceiptTime:
    case CurveAxisConfig::ArrayIndex:
      return static_cast<CurveAxisConfig::FieldType>(raw);
    default:
      return CurveAxisConfig::MessageData;
  }
}

CurveAxisConfig::UnitConversion parseUnitConversion(int raw) {
  switch (raw) {
    case CurveAxisConfig::None:
    case CurveAxisConfig::RadiansToDegrees:
    case CurveAxisConfig::DegreesToRadians:
      return static_cast<CurveAxisConfig::UnitConversion>(raw);
    default:
      return CurveAxisConfig::None;
  }
}

}  // namespace

CurveAxisConfig::CurveAxisConfig(QObject* parent, QString topic, QString type, FieldType fieldType, QString field, bool labelFromZero)
    : Config(parent),
      topic_(std::move(topic)),
      type_(std::move(type)),
      fieldType_(fieldType),
      field_(std::move(field)),
      labelFromZero_(labelFromZero),
      unitConversion_(None),
      scaleConfig_(new CurveAxisScaleConfig(this)) {
  connect(scaleConfig_, SIGNAL(changed()), this, SLOT(scaleChanged()));
}

CurveAxisConfig::~CurveAxisConfig() = default;

void CurveAxisConfig::setTopic(const QString& topic) {
  if (topic != topic_) {
    topic_ = topic;

    emit topicChanged(topic);
    emit changed();
  }
}

const QString& CurveAxisConfig::getTopic() const {
  return topic_;
}

void CurveAxisConfig::setType(const QString& type) {
  if (type != type_) {
    type_ = type;

    emit typeChanged(type);
    emit changed();
  }
}

const QString& CurveAxisConfig::getType() const {
  return type_;
}

void CurveAxisConfig::setFieldType(FieldType fieldType) {
  if (fieldType != fieldType_) {
    fieldType_ = fieldType;

    emit fieldTypeChanged(fieldType);
    emit changed();
  }
}

CurveAxisConfig::FieldType CurveAxisConfig::getFieldType() const {
  return fieldType_;
}

void CurveAxisConfig::setField(const QString& field) {
  if (field != field_) {
    field_ = field;

    emit fieldChanged(field);
    emit changed();
  }
}

const QString& CurveAxisConfig::getField() const {
  return field_;
}

void CurveAxisConfig::setLabelFromZero(bool labelFromZero) {
  if (labelFromZero != labelFromZero_) {
    labelFromZero_ = labelFromZero;

    emit labelFromZeroChanged(labelFromZero);
    emit changed();
  }
}

bool CurveAxisConfig::isLabelFromZero() const {
  return labelFromZero_;
}

void CurveAxisConfig::setUnitConversion(UnitConversion unitConversion) {
  if (unitConversion != unitConversion_) {
    unitConversion_ = unitConversion;

    emit unitConversionChanged(unitConversion);
    emit changed();
  }
}

CurveAxisConfig::UnitConversion CurveAxisConfig::getUnitConversion() const {
  return unitConversion_;
}

double CurveAxisConfig::conversionFactor(UnitConversion unitConversion) {
  switch (unitConversion) {
    case RadiansToDegrees:
      return 180.0 / M_PI;
    case DegreesToRadians:
      return M_PI / 180.0;
    case None:
    default:
      return 1.0;
  }
}

double CurveAxisConfig::convertValue(double value) const {
  if (fieldType_ == ArrayIndex || isTimeSource()) {
    return value;
  }
  return value * conversionFactor(unitConversion_);
}

bool CurveAxisConfig::isTimeFieldPath(const QString& field) {
  return (field == QLatin1String("stamp")) || field.endsWith(QLatin1String("/stamp"));
}

bool CurveAxisConfig::usesTimeScale() const {
  if (fieldType_ == ArrayIndex) {
    return false;
  }
  return labelFromZero_ || (fieldType_ == MessageReceiptTime) || (fieldType_ == MessageData && isTimeFieldPath(field_));
}

bool CurveAxisConfig::isTimeSource() const {
  if (fieldType_ == ArrayIndex) {
    return false;
  }
  return (fieldType_ == MessageReceiptTime) || (fieldType_ == MessageData && isTimeFieldPath(field_));
}

bool CurveAxisConfig::hasConfiguredSource() const {
  return fieldType_ == MessageReceiptTime || fieldType_ == ArrayIndex || !field_.isEmpty();
}

QString CurveAxisConfig::getFieldLabel() const {
  if (fieldType_ == MessageReceiptTime) {
    return QStringLiteral("receipt_time");
  }
  if (fieldType_ == ArrayIndex) {
    return QStringLiteral("index");
  }
  return field_;
}

CurveAxisScaleConfig* CurveAxisConfig::getScaleConfig() const {
  return scaleConfig_;
}

void CurveAxisConfig::save(QSettings& settings) const {
  settings.setValue("topic", topic_);
  settings.setValue("type", type_);
  settings.setValue("field_type", fieldType_);
  settings.setValue("field", field_);
  settings.setValue("label_from_zero", labelFromZero_);
  settings.setValue("unit_conversion", unitConversion_);

  settings.beginGroup("scale");
  scaleConfig_->save(settings);
  settings.endGroup();
}

void CurveAxisConfig::load(QSettings& settings) {
  setTopic(settings.value("topic").toString());
  setType(settings.value("type").toString());
  const auto fieldType = parseFieldType(settings.value("field_type").toInt());
  setFieldType(fieldType);
  setField(settings.value("field").toString());
  setLabelFromZero(settings.value("label_from_zero", fieldType == MessageReceiptTime).toBool());
  setUnitConversion(parseUnitConversion(settings.value("unit_conversion", None).toInt()));

  settings.beginGroup("scale");
  scaleConfig_->load(settings);
  settings.endGroup();
}

void CurveAxisConfig::reset() {
  setTopic(QString());
  setType(QString());
  setFieldType(MessageData);
  setField(QString());
  setLabelFromZero(false);
  setUnitConversion(None);

  scaleConfig_->reset();
}

void CurveAxisConfig::write(QDataStream& stream) const {
  stream << topic_;
  stream << type_;
  stream << (int)fieldType_;
  stream << field_;
  stream << labelFromZero_;
  stream << static_cast<int>(unitConversion_);

  scaleConfig_->write(stream);
}

void CurveAxisConfig::read(QDataStream& stream) {
  QString topic;
  QString type;
  QString field;
  int fieldType = 0;
  bool labelFromZero = false;
  int unitConversion = 0;

  stream >> topic;
  setTopic(topic);
  stream >> type;
  setType(type);
  stream >> fieldType;
  setFieldType(parseFieldType(fieldType));
  stream >> field;
  setField(field);
  stream >> labelFromZero;
  setLabelFromZero(labelFromZero);
  stream >> unitConversion;
  setUnitConversion(parseUnitConversion(unitConversion));

  scaleConfig_->read(stream);
}

CurveAxisConfig& CurveAxisConfig::operator=(const CurveAxisConfig& src) {
  if (this == &src) {
    return *this;
  }

  setTopic(src.topic_);
  setType(src.type_);
  setFieldType(src.fieldType_);
  setField(src.field_);
  setLabelFromZero(src.labelFromZero_);
  setUnitConversion(src.unitConversion_);

  *scaleConfig_ = *src.scaleConfig_;

  return *this;
}

void CurveAxisConfig::scaleChanged() {
  emit changed();
}

}  // namespace rqt_multiplot
