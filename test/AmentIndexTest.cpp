#include <gtest/gtest.h>

#include <QDir>
#include <QString>

#include "rqt_multiplot/AmentIndex.hpp"

namespace {

using rqt_multiplot::packageSharePath;
using rqt_multiplot::readIndexResource;
using rqt_multiplot::resourcesByName;

TEST(AmentIndex, packageSharePathReturnsExistingDirectory) {
  const QString path = QString::fromStdString(packageSharePath("std_msgs"));

  EXPECT_FALSE(path.isEmpty());
  EXPECT_TRUE(QDir(path).exists());
}

TEST(AmentIndex, resourcesByNameListsInterfacePackages) {
  const auto resources = resourcesByName("rosidl_interfaces");

  EXPECT_FALSE(resources.empty());
  EXPECT_NE(resources.find("std_msgs"), resources.end());
}

TEST(AmentIndex, readIndexResourceReturnsInterfaceListing) {
  std::string content;
  ASSERT_TRUE(readIndexResource("rosidl_interfaces", "std_msgs", content));
  EXPECT_NE(content.find("msg/"), std::string::npos);
}

TEST(AmentIndex, readIndexResourceReturnsFalseForUnknownName) {
  std::string content;
  EXPECT_FALSE(readIndexResource("rosidl_interfaces", "not_a_real_package_xyz", content));
}

}  // namespace
