cmake_minimum_required(VERSION 3.16)
project(rqt_multiplot)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(Boost REQUIRED)
find_package(fastcdr REQUIRED)
find_package(pluginlib REQUIRED)
find_package(qt_gui_cpp REQUIRED)
find_package(rcl REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ros_babel_fish REQUIRED)
find_package(rosbag2_cpp REQUIRED)
find_package(rosbag2_storage REQUIRED)
find_package(rosidl_runtime_c REQUIRED)
find_package(rosidl_runtime_cpp REQUIRED)
find_package(rosidl_typesupport_fastrtps_cpp REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)
find_package(rosidl_buffer QUIET)
find_package(rqt_gui_cpp REQUIRED)
find_package(type_description_interfaces REQUIRED)

# Follow the Qt major version already selected by qt_gui_cpp / rqt_gui_cpp.
# Preferring Qt6 here would break jazzy/kilted hosts that also have Qt6 installed.
if(TARGET Qt6::Core OR QT_VERSION_MAJOR EQUAL 6)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Svg OpenGL OpenGLWidgets)
  set(QT_VERSION_MAJOR 6)
else()
  find_package(Qt5 REQUIRED COMPONENTS Widgets Svg)
  set(QT_VERSION_MAJOR 5)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
  find_path(
    QWT_INCLUDE_DIRS
    NAMES
      qwt_plot.h
    PATHS
      /usr/include/qwt
      /usr/include/qwt6
      /usr/include/qt6/qwt
      /usr/local/include/qwt
  )
  find_library(
    QWT_LIBRARIES
    NAMES
      qwt-qt6
  )
  if(NOT QWT_INCLUDE_DIRS OR NOT QWT_LIBRARIES)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchQwtQt6.cmake")
  endif()
else()
  find_path(
    QWT_INCLUDE_DIRS
    NAMES
      qwt_plot.h
    PATHS
      /usr/include/qwt
      /usr/local/include/qwt
  )
  find_library(
    QWT_LIBRARIES
    NAMES
      qwt-qt5
      qwt
  )
  if(NOT QWT_INCLUDE_DIRS OR NOT QWT_LIBRARIES)
    message(FATAL_ERROR "Qwt (Qt5) not found")
  endif()
endif()

set(rqt_multiplot_SRCS
  src/rqt_multiplot/AxisTimeFormat.cpp
  src/rqt_multiplot/BagOpen.cpp
  src/rqt_multiplot/BagQuery.cpp
  src/rqt_multiplot/BagReader.cpp
  src/rqt_multiplot/BagTopicLoader.cpp
  src/rqt_multiplot/BitOperations.cpp
  src/rqt_multiplot/BoundingRectangle.cpp
  src/rqt_multiplot/ColorOperations.cpp
  src/rqt_multiplot/Config.cpp
  src/rqt_multiplot/ConfigComboBox.cpp
  src/rqt_multiplot/CurveAxisConfig.cpp
  src/rqt_multiplot/CurveAxisConfigWidget.cpp
  src/rqt_multiplot/CurveAxisScaleConfig.cpp
  src/rqt_multiplot/CurveAxisScaleConfigWidget.cpp
  src/rqt_multiplot/CurveColorConfig.cpp
  src/rqt_multiplot/CurveColorConfigWidget.cpp
  src/rqt_multiplot/CurveConfig.cpp
  src/rqt_multiplot/CurveConfigDialog.cpp
  src/rqt_multiplot/CurveConfigWidget.cpp
  src/rqt_multiplot/CurveData.cpp
  src/rqt_multiplot/CurveDataCircularBuffer.cpp
  src/rqt_multiplot/CurveDataConfig.cpp
  src/rqt_multiplot/CurveDataConfigWidget.cpp
  src/rqt_multiplot/CurveDataList.cpp
  src/rqt_multiplot/CurveDataListTimeFrame.cpp
  src/rqt_multiplot/CurveDataSequencer.cpp
  src/rqt_multiplot/CurveDataVector.cpp
  src/rqt_multiplot/CurveStatistics.cpp
  src/rqt_multiplot/CurveItemWidget.cpp
  src/rqt_multiplot/CurveListWidget.cpp
  src/rqt_multiplot/CurveStyleConfig.cpp
  src/rqt_multiplot/CurveStyleConfigWidget.cpp
  src/rqt_multiplot/CurveValuesWidget.cpp
  src/rqt_multiplot/DataStatisticsDialog.cpp
  src/rqt_multiplot/DataTypeRegistry.cpp
  src/rqt_multiplot/FileScheme.cpp
  src/rqt_multiplot/MatchFilterComboBox.cpp
  src/rqt_multiplot/MatchFilterCompleter.cpp
  src/rqt_multiplot/MatchFilterCompleterModel.cpp
  src/rqt_multiplot/Message.cpp
  src/rqt_multiplot/MessageBroker.cpp
  src/rqt_multiplot/MessageDefinitionLoader.cpp
  src/rqt_multiplot/MessageEvent.cpp
  src/rqt_multiplot/MessageFieldAccess.cpp
  src/rqt_multiplot/MessageFieldCompleter.cpp
  src/rqt_multiplot/MessageFieldItem.cpp
  src/rqt_multiplot/MessageFieldItemModel.cpp
  src/rqt_multiplot/MessageFieldLineEdit.cpp
  src/rqt_multiplot/MessageFieldTreeWidget.cpp
  src/rqt_multiplot/MessageFieldWidget.cpp
  src/rqt_multiplot/MessageSubscriber.cpp
  src/rqt_multiplot/MessageSubscriberRegistry.cpp
  src/rqt_multiplot/MessageTopicComboBox.cpp
  src/rqt_multiplot/MessageTopicRegistry.cpp
  src/rqt_multiplot/MessageTypeComboBox.cpp
  src/rqt_multiplot/MessageTypeRegistry.cpp
  src/rqt_multiplot/OffsetScaleDraw.cpp
  src/rqt_multiplot/OffsetScaleEngine.cpp
  src/rqt_multiplot/LaunchOptions.cpp
  src/rqt_multiplot/MultiplotConfig.cpp
  src/rqt_multiplot/MultiplotConfigWidget.cpp
  src/rqt_multiplot/MultiplotPlugin.cpp
  src/rqt_multiplot/MultiplotWidget.cpp
  src/rqt_multiplot/StandaloneWindowSettings.cpp
  src/rqt_multiplot/PackageRegistry.cpp
  src/rqt_multiplot/PackageScheme.cpp
  src/rqt_multiplot/PenStyleComboBox.cpp
  src/rqt_multiplot/PenStyleItemDelegate.cpp
  src/rqt_multiplot/PreferencesDialog.cpp
  src/rqt_multiplot/Theme.cpp
  src/rqt_multiplot/PlotAxesConfig.cpp
  src/rqt_multiplot/PlotAxesConfigWidget.cpp
  src/rqt_multiplot/PlotAxisConfig.cpp
  src/rqt_multiplot/PlotAxisConfigWidget.cpp
  src/rqt_multiplot/PlotConfig.cpp
  src/rqt_multiplot/PlotConfigDialog.cpp
  src/rqt_multiplot/PlotConfigWidget.cpp
  src/rqt_multiplot/PlotCursor.cpp
  src/rqt_multiplot/PlotCursorMachine.cpp
  src/rqt_multiplot/PlotExport.cpp
  src/rqt_multiplot/PlotCurve.cpp
  src/rqt_multiplot/PlotLegend.cpp
  src/rqt_multiplot/PlotLegendConfig.cpp
  src/rqt_multiplot/PlotLegendConfigWidget.cpp
  src/rqt_multiplot/PlotLayoutConfig.cpp
  src/rqt_multiplot/PlotMagnifier.cpp
  src/rqt_multiplot/PlotPanner.cpp
  src/rqt_multiplot/PlotCanvasPolicy.cpp
  src/rqt_multiplot/PlotOpenGLCanvas.cpp
  src/rqt_multiplot/PlotSplitter.cpp
  src/rqt_multiplot/PlotTableConfig.cpp
  src/rqt_multiplot/PlotTableConfigWidget.cpp
  src/rqt_multiplot/PlotTableWidget.cpp
  src/rqt_multiplot/PlotTabWidget.cpp
  src/rqt_multiplot/PlotTitleStyle.cpp
  src/rqt_multiplot/PlotWidget.cpp
  src/rqt_multiplot/PlotZoomer.cpp
  src/rqt_multiplot/ProgressChangeEvent.cpp
  src/rqt_multiplot/ProgressWidget.cpp
  src/rqt_multiplot/QwtPlotCustom.cpp
  src/rqt_multiplot/RosContext.cpp
  src/rqt_multiplot/runtime_types/MsgDefinitionParser.cpp
  src/rqt_multiplot/runtime_types/RuntimeCdr.cpp
  src/rqt_multiplot/runtime_types/RuntimeFieldOperations.cpp
  src/rqt_multiplot/runtime_types/RuntimeMessageLayout.cpp
  src/rqt_multiplot/runtime_types/RuntimeSlots.cpp
  src/rqt_multiplot/runtime_types/RuntimeTypeSupport.cpp
  src/rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.cpp
  src/rqt_multiplot/runtime_types/TypeDescriptionClient.cpp
  src/rqt_multiplot/runtime_types/TypeDescriptionModel.cpp
  src/rqt_multiplot/SnapshotHistory.cpp
  src/rqt_multiplot/StatusWidget.cpp
  src/rqt_multiplot/TimeZoneUtil.cpp
  src/rqt_multiplot/TopicBrowserWidget.cpp
  src/rqt_multiplot/TopicFieldMime.cpp
  src/rqt_multiplot/TopicFieldTreeWidget.cpp
  src/rqt_multiplot/TopicSampleLoader.cpp
  src/rqt_multiplot/UserPreferences.cpp
  src/rqt_multiplot/ThreadedTimer.cpp
  src/rqt_multiplot/UrlComboBox.cpp
  src/rqt_multiplot/UrlCompleter.cpp
  src/rqt_multiplot/UrlItem.cpp
  src/rqt_multiplot/UrlItemModel.cpp
  src/rqt_multiplot/UrlScheme.cpp
  src/rqt_multiplot/XmlSettings.cpp
)

set(rqt_multiplot_HDRS
  include/rqt_multiplot/AxisTimeFormat.hpp
  include/rqt_multiplot/BagOpen.hpp
  include/rqt_multiplot/BagQuery.hpp
  include/rqt_multiplot/BagReader.hpp
  include/rqt_multiplot/BagTopicLoader.hpp
  include/rqt_multiplot/BitOperations.hpp
  include/rqt_multiplot/BoundingRectangle.hpp
  include/rqt_multiplot/ColorOperations.hpp
  include/rqt_multiplot/Config.hpp
  include/rqt_multiplot/ConfigComboBox.hpp
  include/rqt_multiplot/CurveAxisConfig.hpp
  include/rqt_multiplot/CurveAxisConfigWidget.hpp
  include/rqt_multiplot/CurveAxisScaleConfig.hpp
  include/rqt_multiplot/CurveAxisScaleConfigWidget.hpp
  include/rqt_multiplot/CurveColorConfig.hpp
  include/rqt_multiplot/CurveColorConfigWidget.hpp
  include/rqt_multiplot/CurveConfig.hpp
  include/rqt_multiplot/CurveConfigDialog.hpp
  include/rqt_multiplot/CurveConfigWidget.hpp
  include/rqt_multiplot/CurveData.hpp
  include/rqt_multiplot/CurveDataCircularBuffer.hpp
  include/rqt_multiplot/CurveDataConfig.hpp
  include/rqt_multiplot/CurveDataConfigWidget.hpp
  include/rqt_multiplot/CurveDataList.hpp
  include/rqt_multiplot/CurveDataListTimeFrame.hpp
  include/rqt_multiplot/CurveDataSequencer.hpp
  include/rqt_multiplot/CurveDataVector.hpp
  include/rqt_multiplot/CurveItemWidget.hpp
  include/rqt_multiplot/CurveListWidget.hpp
  include/rqt_multiplot/CurveStyleConfig.hpp
  include/rqt_multiplot/CurveStyleConfigWidget.hpp
  include/rqt_multiplot/CurveValuesWidget.hpp
  include/rqt_multiplot/DataTypeRegistry.hpp
  include/rqt_multiplot/FileScheme.hpp
  include/rqt_multiplot/MatchFilterComboBox.hpp
  include/rqt_multiplot/MatchFilterCompleter.hpp
  include/rqt_multiplot/MatchFilterCompleterModel.hpp
  include/rqt_multiplot/Message.hpp
  include/rqt_multiplot/MessageBroker.hpp
  include/rqt_multiplot/MessageDefinitionLoader.hpp
  include/rqt_multiplot/MessageEvent.hpp
  include/rqt_multiplot/MessageFieldCompleter.hpp
  include/rqt_multiplot/MessageFieldItem.hpp
  include/rqt_multiplot/MessageFieldItemModel.hpp
  include/rqt_multiplot/MessageFieldLineEdit.hpp
  include/rqt_multiplot/MessageFieldTreeWidget.hpp
  include/rqt_multiplot/MessageFieldWidget.hpp
  include/rqt_multiplot/MessageSubscriber.hpp
  include/rqt_multiplot/MessageSubscriberRegistry.hpp
  include/rqt_multiplot/MessageTopicComboBox.hpp
  include/rqt_multiplot/MessageTopicRegistry.hpp
  include/rqt_multiplot/MessageTypeComboBox.hpp
  include/rqt_multiplot/MessageTypeRegistry.hpp
  include/rqt_multiplot/OffsetScaleDraw.hpp
  include/rqt_multiplot/OffsetScaleEngine.hpp
  include/rqt_multiplot/MultiplotConfig.hpp
  include/rqt_multiplot/MultiplotConfigWidget.hpp
  include/rqt_multiplot/MultiplotPlugin.hpp
  include/rqt_multiplot/MultiplotWidget.hpp
  include/rqt_multiplot/PackageRegistry.hpp
  include/rqt_multiplot/PackageScheme.hpp
  include/rqt_multiplot/PenStyleComboBox.hpp
  include/rqt_multiplot/PenStyleItemDelegate.hpp
  include/rqt_multiplot/PreferencesDialog.hpp
  include/rqt_multiplot/PlotAxesConfig.hpp
  include/rqt_multiplot/PlotAxesConfigWidget.hpp
  include/rqt_multiplot/PlotAxisConfig.hpp
  include/rqt_multiplot/PlotAxisConfigWidget.hpp
  include/rqt_multiplot/PlotConfig.hpp
  include/rqt_multiplot/PlotConfigDialog.hpp
  include/rqt_multiplot/PlotConfigWidget.hpp
  include/rqt_multiplot/PlotCursor.hpp
  include/rqt_multiplot/PlotCursorLabel.hpp
  include/rqt_multiplot/PlotCursorMachine.hpp
  include/rqt_multiplot/PlotExport.hpp
  include/rqt_multiplot/PlotCurve.hpp
  include/rqt_multiplot/PlotLegend.hpp
  include/rqt_multiplot/PlotLegendStyle.hpp
  include/rqt_multiplot/PlotLegendConfig.hpp
  include/rqt_multiplot/PlotLegendConfigWidget.hpp
  include/rqt_multiplot/PlotLayoutConfig.hpp
  include/rqt_multiplot/PlotMagnifier.hpp
  include/rqt_multiplot/PlotMouseBindings.hpp
  include/rqt_multiplot/PlotCanvasPolicy.hpp
  include/rqt_multiplot/PlotOpenGLCanvas.hpp
  include/rqt_multiplot/PlotReplotPolicy.hpp
  include/rqt_multiplot/PlotPanner.hpp
  include/rqt_multiplot/PlotSplitter.hpp
  include/rqt_multiplot/PlotTableConfig.hpp
  include/rqt_multiplot/PlotTableConfigWidget.hpp
  include/rqt_multiplot/PlotTableWidget.hpp
  include/rqt_multiplot/PlotTabWidget.hpp
  include/rqt_multiplot/PlotTitleStyle.hpp
  include/rqt_multiplot/PlotWidget.hpp
  include/rqt_multiplot/PlotZoomer.hpp
  include/rqt_multiplot/ProgressChangeEvent.hpp
  include/rqt_multiplot/ProgressWidget.hpp
  include/rqt_multiplot/QwtPlotCustom.hpp
  include/rqt_multiplot/SnapshotHistory.hpp
  include/rqt_multiplot/RosContext.hpp
  include/rqt_multiplot/runtime_types/MsgDefinitionParser.hpp
  include/rqt_multiplot/runtime_types/RuntimeCdr.hpp
  include/rqt_multiplot/runtime_types/RuntimeFieldOperations.hpp
  include/rqt_multiplot/runtime_types/RuntimeMessageLayout.hpp
  include/rqt_multiplot/runtime_types/RuntimeSlots.hpp
  include/rqt_multiplot/runtime_types/RuntimeTypeSupport.hpp
  include/rqt_multiplot/runtime_types/RuntimeTypeSupportProvider.hpp
  include/rqt_multiplot/runtime_types/RuntimeValueTypes.hpp
  include/rqt_multiplot/runtime_types/TypeDescriptionClient.hpp
  include/rqt_multiplot/runtime_types/TypeDescriptionModel.hpp
  include/rqt_multiplot/StatusWidget.hpp
  include/rqt_multiplot/Theme.hpp
  include/rqt_multiplot/TimeZoneUtil.hpp
  include/rqt_multiplot/TopicBrowserWidget.hpp
  include/rqt_multiplot/TopicFieldMime.hpp
  include/rqt_multiplot/TopicFieldTreeWidget.hpp
  include/rqt_multiplot/TopicSampleLoader.hpp
  include/rqt_multiplot/UserPreferences.hpp
  include/rqt_multiplot/ThreadedTimer.hpp
  include/rqt_multiplot/UrlComboBox.hpp
  include/rqt_multiplot/UrlCompleter.hpp
  include/rqt_multiplot/UrlItem.hpp
  include/rqt_multiplot/UrlItemModel.hpp
  include/rqt_multiplot/UrlScheme.hpp
  include/rqt_multiplot/XmlSettings.hpp
)

set(rqt_multiplot_UIS
  src/rqt_multiplot/CurveAxisConfigWidget.ui
  src/rqt_multiplot/CurveAxisScaleConfigWidget.ui
  src/rqt_multiplot/CurveColorConfigWidget.ui
  src/rqt_multiplot/CurveConfigDialog.ui
  src/rqt_multiplot/CurveConfigWidget.ui
  src/rqt_multiplot/CurveDataConfigWidget.ui
  src/rqt_multiplot/CurveItemWidget.ui
  src/rqt_multiplot/CurveStyleConfigWidget.ui
  src/rqt_multiplot/MessageFieldWidget.ui
  src/rqt_multiplot/MultiplotConfigWidget.ui
  src/rqt_multiplot/MultiplotWidget.ui
  src/rqt_multiplot/PreferencesDialog.ui
  src/rqt_multiplot/PlotAxesConfigWidget.ui
  src/rqt_multiplot/PlotAxisConfigWidget.ui
  src/rqt_multiplot/PlotConfigDialog.ui
  src/rqt_multiplot/PlotConfigWidget.ui
  src/rqt_multiplot/PlotLegendConfigWidget.ui
  src/rqt_multiplot/PlotTableConfigWidget.ui
  src/rqt_multiplot/PlotWidget.ui
  src/rqt_multiplot/ProgressWidget.ui
)

qt_wrap_cpp(rqt_multiplot_MOCS ${rqt_multiplot_HDRS})
qt_wrap_ui(rqt_multiplot_UIS_H ${rqt_multiplot_UIS})

add_library(${PROJECT_NAME} SHARED
  ${rqt_multiplot_SRCS}
  ${rqt_multiplot_MOCS}
  ${rqt_multiplot_UIS_H}
)

target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>"
  ${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
  ${pluginlib_TARGETS}
  ${qt_gui_cpp_TARGETS}
  ${rqt_gui_cpp_TARGETS}
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::Svg
  ament_index_cpp::ament_index_cpp
  fastcdr
  rcl::rcl
  rclcpp::rclcpp
  ros_babel_fish::ros_babel_fish
  rosbag2_cpp::rosbag2_cpp
  rosbag2_storage::rosbag2_storage
  rosidl_runtime_c::rosidl_runtime_c
  rosidl_runtime_cpp::rosidl_runtime_cpp
  rosidl_typesupport_fastrtps_cpp::rosidl_typesupport_fastrtps_cpp
  rosidl_typesupport_introspection_cpp::rosidl_typesupport_introspection_cpp
  ${type_description_interfaces_TARGETS}
)

if(TARGET rosidl_buffer::rosidl_buffer)
  target_link_libraries(${PROJECT_NAME} PUBLIC rosidl_buffer::rosidl_buffer)
endif()

if(QT_VERSION_MAJOR EQUAL 6)
  target_link_libraries(${PROJECT_NAME} PUBLIC Qt6::OpenGLWidgets)
endif()

if(TARGET qwt_qt6)
  target_link_libraries(${PROJECT_NAME} PRIVATE qwt_qt6)
else()
  target_include_directories(${PROJECT_NAME} PUBLIC ${QWT_INCLUDE_DIRS})
  target_link_libraries(${PROJECT_NAME} PUBLIC ${QWT_LIBRARIES})
endif()

add_executable(multiplot
  src/rqt_multiplot/multiplot_main.cpp
)

target_link_libraries(multiplot PRIVATE
  ${PROJECT_NAME}
  Qt${QT_VERSION_MAJOR}::Widgets
  rclcpp::rclcpp
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

install(TARGETS multiplot
  DESTINATION lib/${PROJECT_NAME}
)

install(PROGRAMS
  scripts/rqt_multiplot
  scripts/publish_array_demo.py
  scripts/publish_diagnostics_demo.py
  DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include/${PROJECT_NAME}
)

install(FILES plugin.xml
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY resource
  DESTINATION share/${PROJECT_NAME}
)

pluginlib_export_plugin_description_file(rqt_gui plugin.xml)

ament_export_include_directories("include/${PROJECT_NAME}")
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(ament_cmake_test REQUIRED)
  find_package(builtin_interfaces REQUIRED)
  find_package(diagnostic_msgs REQUIRED)
  find_package(geometry_msgs REQUIRED)
  find_package(sensor_msgs REQUIRED)
  find_package(std_msgs REQUIRED)
  find_package(test_msgs REQUIRED)

  ament_add_gtest(test_${PROJECT_NAME}
    test/AxisTimeFormatTest.cpp
    test/BagReaderTest.cpp
    test/BagTopicLoaderTest.cpp
    test/CurveAxisConfigTest.cpp
    test/CurveStyleConfigTest.cpp
    test/CurveStyleConfigWidgetTest.cpp
    test/CurveConfigWidgetTest.cpp
    test/CurveDataSequencerTest.cpp
    test/CurveDataPolicyTest.cpp
    test/CurveDataVectorTest.cpp
    test/CurveStatisticsTest.cpp
    test/CurveValuesWidgetTest.cpp
    test/DataStatisticsDialogTest.cpp
    test/MessageFieldAccessTest.cpp
    test/MessageSubscriberTest.cpp
    test/MessageTopicRegistryTest.cpp
    test/OffsetScaleDrawTest.cpp
    test/LaunchOptionsTest.cpp
    test/MultiplotConfigTest.cpp
    test/MultiplotConfigWidgetTest.cpp
    test/MultiplotWidgetTest.cpp
    test/MultiplotFileMenuTest.cpp
    test/StandaloneWindowSettingsTest.cpp
    test/PlotConfigTest.cpp
    test/PlotConfigWidgetTest.cpp
    test/PlotLayoutConfigTest.cpp
    test/PlotTableConfigTest.cpp
    test/PlotTableWidgetLayoutTest.cpp
    test/PlotSplitterHandleTest.cpp
    test/PlotCurveTest.cpp
    test/PlotLegendTest.cpp
    test/PlotTitleStyleTest.cpp
    test/PlotWidgetTitleStyleTest.cpp
    test/PlotWidgetSplitMenuTest.cpp
    test/PlotWidgetContextMenuTest.cpp
    test/PlotWidgetTopicDropTest.cpp
    test/PlotCursorLabelTest.cpp
    test/SnapshotHistoryTest.cpp
    test/StatusWidgetTest.cpp
    test/ThemeTest.cpp
    test/TimeZoneUtilTest.cpp
    test/TopicBrowserWidgetTest.cpp
    test/TopicFieldMimeTest.cpp
    test/TopicSampleLoaderTest.cpp
    test/UserPreferencesTest.cpp
    test/UrlComboBoxTest.cpp
    test/PlotExportTest.cpp
    test/PlotMouseBindingsTest.cpp
    test/PlotReplotPolicyTest.cpp
    test/PlotCanvasPolicyTest.cpp
    test/QwtPlotCustomTest.cpp
    test/PlotTabWidgetTest.cpp
    test/PreferencesDialogTest.cpp
    test/AmentIndexTest.cpp
    test/PackageResourceTest.cpp
    test/MatchFilterCompleterModelTest.cpp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
  )
  set_tests_properties(test_${PROJECT_NAME} PROPERTIES LABELS "unit_testing")
  if(TARGET test_${PROJECT_NAME})
    target_link_libraries(test_${PROJECT_NAME}
      ${PROJECT_NAME}
      Qt${QT_VERSION_MAJOR}::Core
      Qt${QT_VERSION_MAJOR}::Widgets
      ${diagnostic_msgs_TARGETS}
      ${geometry_msgs_TARGETS}
      ${sensor_msgs_TARGETS}
      ${std_msgs_TARGETS}
      ${test_msgs_TARGETS}
      rosbag2_cpp::rosbag2_cpp
      rosbag2_storage::rosbag2_storage
    )
    if(TARGET qwt_qt6)
      target_link_libraries(test_${PROJECT_NAME} qwt_qt6)
    endif()
  endif()

  ament_add_gtest_executable(test_runtime_types
    test/MsgDefinitionParserTest.cpp
    test/RuntimeTypeSupportProviderTest.cpp
    test/RuntimeTypeSupportTest.cpp
  )
  target_link_libraries(test_runtime_types
    ${PROJECT_NAME}
    ${diagnostic_msgs_TARGETS}
    ${sensor_msgs_TARGETS}
    ${test_msgs_TARGETS}
    rosbag2_cpp::rosbag2_cpp
    rosbag2_storage::rosbag2_storage
  )
  foreach(rmw_implementation rmw_fastrtps_cpp rmw_cyclonedds_cpp rmw_zenoh_cpp)
    find_package(${rmw_implementation} QUIET)
    if(${rmw_implementation}_FOUND)
      ament_add_gtest_test(test_runtime_types
        TEST_NAME test_runtime_types_${rmw_implementation}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test
        ENV RMW_IMPLEMENTATION=${rmw_implementation}
      )
      set_tests_properties(test_runtime_types_${rmw_implementation} PROPERTIES LABELS "unit_testing")
    endif()
  endforeach()

  find_package(ament_cmake_clang_format REQUIRED)
  ament_clang_format(
    src include test
    CONFIG_FILE ${CMAKE_CURRENT_SOURCE_DIR}/.clang-format
    --clang-format-version 23
  )

  ament_add_test(test_run_clang_tidy
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test/test_run_clang_tidy.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 30
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  set_tests_properties(test_run_clang_tidy PROPERTIES LABELS "unit_testing_clang_tidy")

  ament_add_test(clang_tidy
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_clang_tidy.py
      --compile-commands ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
      --source-root ${CMAKE_CURRENT_SOURCE_DIR}
      --config ${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 1800
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
  )
  set_tests_properties(clang_tidy PROPERTIES LABELS "clang_tidy;linting")
endif()

ament_package()
