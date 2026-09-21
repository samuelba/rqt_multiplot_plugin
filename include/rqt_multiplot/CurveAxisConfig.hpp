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

#pragma once

#include <QString>

#include "rqt_multiplot/Config.hpp"
#include "rqt_multiplot/CurveAxisScaleConfig.hpp"

namespace rqt_multiplot {

class CurveAxisConfig : public Config {
  Q_OBJECT
 public:
  enum FieldType { MessageData, MessageReceiptTime, ArrayIndex };
  enum UnitConversion { None, RadiansToDegrees, DegreesToRadians };

  explicit CurveAxisConfig(QObject* parent = nullptr, QString topic = QString(), QString type = QString(),
                           FieldType fieldType = MessageData, QString field = QString(), bool labelFromZero = false);
  ~CurveAxisConfig() override;

  void setTopic(const QString& topic);
  const QString& getTopic() const;
  void setType(const QString& type);
  const QString& getType() const;
  void setFieldType(FieldType fieldType);
  FieldType getFieldType() const;
  void setField(const QString& field);
  const QString& getField() const;
  void setLabelFromZero(bool labelFromZero);
  bool isLabelFromZero() const;
  void setUnitConversion(UnitConversion unitConversion);
  UnitConversion getUnitConversion() const;
  double convertValue(double value) const;
  static double conversionFactor(UnitConversion unitConversion);
  bool usesTimeScale() const;
  bool isTimeSource() const;
  bool hasConfiguredSource() const;
  QString getFieldLabel() const;
  static bool isTimeFieldPath(const QString& field);
  CurveAxisScaleConfig* getScaleConfig() const;

  void save(QSettings& settings) const override;
  void load(QSettings& settings) override;
  void reset() override;

  void write(QDataStream& stream) const override;
  void read(QDataStream& stream) override;

  CurveAxisConfig& operator=(const CurveAxisConfig& src);

 signals:
  void topicChanged(const QString& topic);
  void typeChanged(const QString& type);
  void fieldTypeChanged(int fieldType);
  void fieldChanged(const QString& field);
  void labelFromZeroChanged(bool labelFromZero);
  void unitConversionChanged(int unitConversion);

 private:
  QString topic_;
  QString type_;
  FieldType fieldType_;
  QString field_;
  bool labelFromZero_;
  UnitConversion unitConversion_;

  CurveAxisScaleConfig* scaleConfig_;

 private slots:
  void scaleChanged();
};

}  // namespace rqt_multiplot
