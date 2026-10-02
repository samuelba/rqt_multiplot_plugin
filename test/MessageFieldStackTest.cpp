#include <memory>

#include <QApplication>
#include <QSignalSpy>

#include <gtest/gtest.h>

#include "rqt_multiplot/MessageFieldCompleter.hpp"
#include "rqt_multiplot/MessageFieldItemModel.hpp"
#include "rqt_multiplot/MessageFieldLineEdit.hpp"
#include "rqt_multiplot/MessageFieldTreeWidget.hpp"
#include "rqt_multiplot/MessageFieldType.hpp"
#include "rqt_multiplot/MessageFieldWidget.hpp"

namespace {

using rqt_multiplot::MessageFieldCompleter;
using rqt_multiplot::MessageFieldItemModel;
using rqt_multiplot::MessageFieldLineEdit;
using rqt_multiplot::MessageFieldTreeWidget;
using rqt_multiplot::MessageFieldType;
using rqt_multiplot::MessageFieldWidget;

QApplication* ensureApplication() {
  if (QApplication::instance() != nullptr) {
    return qobject_cast<QApplication*>(QApplication::instance());
  }
  qputenv("QT_QPA_PLATFORM", "offscreen");
  static int argc = 1;
  static char arg0[] = "test_rqt_multiplot";
  static char* argv[] = {arg0, nullptr};
  return new QApplication(argc, argv);
}

MessageFieldType numericType() {
  MessageFieldType type;
  type.kind = MessageFieldType::Builtin;
  type.identifier = QStringLiteral("float64");
  type.isNumeric = true;
  return type;
}

MessageFieldType sampleType() {
  MessageFieldType point;
  point.kind = MessageFieldType::Compound;
  point.identifier = QStringLiteral("geometry_msgs/msg/Point");
  point.members = {{QStringLiteral("x"), numericType()}, {QStringLiteral("y"), numericType()}};

  MessageFieldType poses;
  poses.kind = MessageFieldType::Array;
  poses.isDynamicArray = true;
  poses.elementType = std::make_shared<MessageFieldType>(point);

  MessageFieldType message;
  message.kind = MessageFieldType::Compound;
  message.identifier = QStringLiteral("geometry_msgs/msg/PoseArray");
  message.members = {{QStringLiteral("poses"), poses}};
  return message;
}

}  // namespace

TEST(MessageFieldStack, modelTreeAndCompleterResolveANestedField) {
  ensureApplication();
  const MessageFieldType type = sampleType();

  MessageFieldItemModel model;
  model.setMessageDataType(type);
  EXPECT_GT(model.rowCount(QModelIndex()), 0);
  EXPECT_EQ(model.columnCount(QModelIndex()), 1);
  EXPECT_TRUE(model.getFieldDataType(QStringLiteral("poses/*/x")).isNumeric);

  const QModelIndex poses = model.index(0, 0, QModelIndex());
  ASSERT_TRUE(poses.isValid());
  EXPECT_EQ(model.data(poses, Qt::DisplayRole).toString(), QStringLiteral("poses"));
  EXPECT_EQ(model.parent(poses), QModelIndex());
  model.update(QStringLiteral("poses/12/x"));
  EXPECT_TRUE(model.getFieldDataType(QStringLiteral("poses/12/x")).isNumeric);

  MessageFieldCompleter completer(&model);
  const QStringList parts = completer.splitPath(QStringLiteral("poses/12/x"));
  EXPECT_EQ(parts, (QStringList{QStringLiteral("poses"), QStringLiteral("12"), QStringLiteral("x")}));
  EXPECT_FALSE(completer.pathFromIndex(poses).isEmpty());

  MessageFieldTreeWidget tree;
  tree.setMessageDataType(type);
  tree.setCurrentField(QStringLiteral("poses/1/x"));
  EXPECT_EQ(tree.getCurrentField(), QStringLiteral("poses/1/x"));
  EXPECT_TRUE(tree.isCurrentFieldDefined());
  EXPECT_TRUE(tree.getCurrentFieldDataType().isNumeric);
  if (tree.topLevelItemCount() > 0) {
    tree.QTreeWidget::setCurrentItem(tree.topLevelItem(0));
    EXPECT_FALSE(tree.getCurrentField().isEmpty());
  }

  MessageFieldLineEdit edit;
  edit.setMessageDataType(type);
  edit.setCurrentField(QStringLiteral("poses/*/y"));
  EXPECT_TRUE(edit.isCurrentFieldDefined());
  edit.setText(QStringLiteral("poses/2/x"));
  QMetaObject::invokeMethod(&edit, "editingFinished");
  EXPECT_EQ(edit.getCurrentField(), QStringLiteral("poses/2/x"));
}

TEST(MessageFieldStack, widgetLoadsAKnownTypeAndReportsAnUnknownType) {
  ensureApplication();
  MessageFieldWidget widget;

  QSignalSpy failed(&widget, &MessageFieldWidget::loadingFailed);
  widget.loadFields(QStringLiteral("not_a_real_msgs/msg/Missing"));
  ASSERT_TRUE(failed.wait(5000));
  EXPECT_FALSE(widget.isLoading());

  widget.setCurrentField(QStringLiteral("data"));
  QSignalSpy finished(&widget, &MessageFieldWidget::loadingFinished);
  widget.loadFields(QStringLiteral("std_msgs/msg/Float64"));
  ASSERT_TRUE(finished.wait(5000));
  EXPECT_EQ(widget.getCurrentMessageType(), QStringLiteral("std_msgs/msg/Float64"));
  EXPECT_EQ(widget.getCurrentField(), QStringLiteral("data"));
  EXPECT_TRUE(widget.isCurrentFieldDefined());
}
