# Ubuntu 26.04 (lyrical/rolling) ships Qwt 6.1 for Qt5 only. Build Qwt 6.3
# against Qt6 when a system qwt-qt6 library is not present.
#
# Use the multi-argument FetchContent_Populate form. The single-argument
# form is removed under CMake 4 CMP0169.

include(FetchContent)

set(RQT_MULTIPLOT_QWT_VERSION 6.3.0)
set(RQT_MULTIPLOT_QWT_SHA256 dcb085896c28aaec5518cbc08c0ee2b4e60ada7ac929d82639f6189851a6129a)

FetchContent_Populate(
  qwt
  QUIET
  URL https://downloads.sourceforge.net/project/qwt/qwt/${RQT_MULTIPLOT_QWT_VERSION}/qwt-${RQT_MULTIPLOT_QWT_VERSION}.tar.bz2
  URL_HASH SHA256=${RQT_MULTIPLOT_QWT_SHA256}
  SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/qwt-src"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/qwt-build"
  SUBBUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps/qwt-subbuild"
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

find_package(Qt6 REQUIRED COMPONENTS Concurrent OpenGL OpenGLWidgets PrintSupport Svg Widgets)

file(GLOB QWT_SOURCES CONFIGURE_DEPENDS "${qwt_SOURCE_DIR}/src/*.cpp")
list(FILTER QWT_SOURCES EXCLUDE REGEX "qwt_polar_|qwt_plot_glcanvas")

set(_qwt_include_root "${qwt_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${_qwt_include_root}")
if(NOT EXISTS "${_qwt_include_root}/qwt")
  file(CREATE_LINK "${qwt_SOURCE_DIR}/src" "${_qwt_include_root}/qwt" SYMBOLIC)
endif()

add_library(qwt_qt6 STATIC ${QWT_SOURCES})
set_target_properties(qwt_qt6 PROPERTIES
  AUTOMOC ON
  POSITION_INDEPENDENT_CODE ON
)
target_compile_options(qwt_qt6 PRIVATE -w)
target_include_directories(qwt_qt6 PUBLIC
  "${_qwt_include_root}"
  "${qwt_SOURCE_DIR}/src"
)
target_link_libraries(qwt_qt6 PUBLIC
  Qt6::Concurrent
  Qt6::OpenGL
  Qt6::OpenGLWidgets
  Qt6::PrintSupport
  Qt6::Svg
  Qt6::Widgets
)

set(QWT_INCLUDE_DIRS "${_qwt_include_root}")
set(QWT_LIBRARIES qwt_qt6)
