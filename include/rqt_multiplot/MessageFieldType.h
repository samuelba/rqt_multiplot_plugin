/******************************************************************************
 * Copyright (C) 2015 by Ralf Kaestner                                        *
 * ralf.kaestner@gmail.com                                                    *
 ******************************************************************************/

#ifndef RQT_MULTIPLOT_MESSAGE_FIELD_TYPE_H
#define RQT_MULTIPLOT_MESSAGE_FIELD_TYPE_H

#include <memory>

#include <QMetaType>
#include <QPair>
#include <QString>
#include <QVector>

namespace rqt_multiplot {

class MessageFieldType {
 public:
  enum Kind { Invalid, Builtin, Compound, Array };

  Kind kind = Invalid;
  QString identifier;
  bool isNumeric = false;
  bool isTime = false;
  bool isDynamicArray = false;
  size_t arraySize = 0;
  QVector<QPair<QString, MessageFieldType>> members;
  std::shared_ptr<MessageFieldType> elementType;

  bool isValid() const { return kind != Invalid; }
  bool isBuiltin() const { return kind == Builtin; }
  bool isMessage() const { return kind == Compound; }
  bool isArray() const { return kind == Array; }
};

}  // namespace rqt_multiplot

Q_DECLARE_METATYPE(rqt_multiplot::MessageFieldType)

#endif
