#pragma once

#include <string>

#include <rosbag2_cpp/message_definitions/local_message_definition_source.hpp>

namespace rqt_multiplot::test {

template <typename Source>
auto fullMessageDefinition(Source& source, const std::string& type, int /*preferExt*/)
    -> decltype(source.get_full_text_ext(type, std::string{})) {
  return source.get_full_text_ext(type, std::string{});
}

template <typename Source>
rosbag2_storage::MessageDefinition fullMessageDefinition(Source& source, const std::string& type, long /*legacy*/) {
  return source.get_full_text(type);
}

inline rosbag2_storage::MessageDefinition fullMessageDefinition(const std::string& type) {
  rosbag2_cpp::LocalMessageDefinitionSource source;
  return fullMessageDefinition(source, type, 0);
}

}  // namespace rqt_multiplot::test
