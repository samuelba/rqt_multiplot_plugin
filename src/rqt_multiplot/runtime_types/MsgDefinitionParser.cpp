#include "rqt_multiplot/runtime_types/MsgDefinitionParser.hpp"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace rqt_multiplot {

namespace runtime_types {

namespace {

constexpr const char* kMsgPrefix = "MSG: ";
constexpr size_t kMinSeparatorLength = 10;
constexpr const char* kEmptyMessageFieldName = "structure_needs_at_least_one_member";

struct Section {
  std::string typeName;
  std::vector<std::string> lines;
};

std::string trim(const std::string& text) {
  const auto begin = text.find_first_not_of(" \t\r");
  if (begin == std::string::npos) {
    return {};
  }
  const auto end = text.find_last_not_of(" \t\r");
  return text.substr(begin, end - begin + 1);
}

bool isSeparator(const std::string& line) {
  return line.size() >= kMinSeparatorLength && std::all_of(line.begin(), line.end(), [](char c) { return c == '='; });
}

std::string stripComment(const std::string& line) {
  char quote = 0;
  for (size_t i = 0; i < line.size(); ++i) {
    const char c = line[i];
    if (quote != 0) {
      if (c == '\\') {
        ++i;
      } else if (c == quote) {
        quote = 0;
      }
    } else if (c == '"' || c == '\'') {
      quote = c;
    } else if (c == '#') {
      return line.substr(0, i);
    }
  }
  return line;
}

std::string packageOf(const std::string& typeName) {
  return typeName.substr(0, typeName.find('/'));
}

std::string fullTypeName(const std::string& name, const std::string& currentPackage) {
  std::vector<std::string> parts;
  std::stringstream stream(name);
  for (std::string part; std::getline(stream, part, '/');) {
    parts.push_back(part);
  }
  if (parts.size() == 1 && !currentPackage.empty()) {
    return currentPackage + "/msg/" + parts[0];
  }
  if (parts.size() == 2) {
    return parts[0] + "/msg/" + parts[1];
  }
  if (parts.size() == 3) {
    return name;
  }
  throw std::invalid_argument("invalid type name [" + name + "]");
}

uint64_t parseNumber(const std::string& text, const std::string& token) {
  if (text.empty() || !std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
    throw std::invalid_argument("invalid size in type [" + token + "]");
  }
  return std::stoull(text);
}

const std::map<std::string, uint8_t>& primitiveTypes() {
  static const std::map<std::string, uint8_t> types{
      {"bool", field_type::kBoolean},   {"byte", field_type::kByte},     {"char", field_type::kUint8},     {"float32", field_type::kFloat},
      {"float64", field_type::kDouble}, {"int8", field_type::kInt8},     {"uint8", field_type::kUint8},    {"int16", field_type::kInt16},
      {"uint16", field_type::kUint16},  {"int32", field_type::kInt32},   {"uint32", field_type::kUint32},  {"int64", field_type::kInt64},
      {"uint64", field_type::kUint64},  {"string", field_type::kString}, {"wstring", field_type::kWString}};
  return types;
}

FieldTypeModel parseFieldType(const std::string& token, const std::string& package) {
  FieldTypeModel type;
  ContainerKind container = ContainerKind::None;
  std::string base = token;
  const auto bracket = token.find('[');
  if (bracket != std::string::npos) {
    if (token.back() != ']') {
      throw std::invalid_argument("invalid array type [" + token + "]");
    }
    const std::string inside = token.substr(bracket + 1, token.size() - bracket - 2);
    base = token.substr(0, bracket);
    if (inside.empty()) {
      container = ContainerKind::UnboundedSequence;
    } else if (inside.rfind("<=", 0) == 0) {
      container = ContainerKind::BoundedSequence;
      type.capacity = parseNumber(inside.substr(2), token);
    } else {
      container = ContainerKind::Array;
      type.capacity = parseNumber(inside, token);
    }
  }
  const auto stringBound = base.find("<=");
  if (stringBound != std::string::npos) {
    type.stringCapacity = parseNumber(base.substr(stringBound + 2), token);
    base = base.substr(0, stringBound);
  }

  uint8_t baseType = field_type::kNested;
  const auto primitive = primitiveTypes().find(base);
  if (primitive != primitiveTypes().end()) {
    baseType = primitive->second;
    if (type.stringCapacity > 0) {
      if (baseType == field_type::kString) {
        baseType = field_type::kBoundedString;
      } else if (baseType == field_type::kWString) {
        baseType = field_type::kBoundedWString;
      } else {
        throw std::invalid_argument("only strings can be bounded [" + token + "]");
      }
    }
  } else {
    if (type.stringCapacity > 0) {
      throw std::invalid_argument("only strings can be bounded [" + token + "]");
    }
    type.nestedTypeName = fullTypeName(base, package);
  }
  type.typeId = composeTypeId(baseType, container);
  return type;
}

void parseLine(const std::string& line, const std::string& package, IndividualTypeModel& model) {
  const auto typeEnd = line.find_first_of(" \t");
  if (typeEnd == std::string::npos) {
    throw std::invalid_argument("invalid field [" + line + "] in [" + model.typeName + "]");
  }
  const std::string rest = trim(line.substr(typeEnd));
  size_t nameEnd = 0;
  while (nameEnd < rest.size() && (std::isalnum(static_cast<unsigned char>(rest[nameEnd])) != 0 || rest[nameEnd] == '_')) {
    ++nameEnd;
  }
  if (nameEnd == 0) {
    throw std::invalid_argument("invalid field [" + line + "] in [" + model.typeName + "]");
  }
  const std::string tail = trim(rest.substr(nameEnd));
  if (!tail.empty() && tail.front() == '=') {
    return;
  }
  FieldModel field;
  field.name = rest.substr(0, nameEnd);
  field.type = parseFieldType(line.substr(0, typeEnd), package);
  field.defaultValue = tail;
  model.fields.push_back(std::move(field));
}

std::vector<Section> splitSections(const std::string& rootTypeName, const std::string& definition) {
  std::vector<Section> sections{Section{rootTypeName, {}}};
  std::istringstream stream(definition);
  for (std::string line; std::getline(stream, line);) {
    const std::string trimmed = trim(line);
    if (isSeparator(trimmed)) {
      sections.push_back(Section{});
      continue;
    }
    Section& section = sections.back();
    if (!section.typeName.empty()) {
      section.lines.push_back(line);
    } else if (trimmed.rfind(kMsgPrefix, 0) == 0) {
      section.typeName = fullTypeName(trim(trimmed.substr(std::char_traits<char>::length(kMsgPrefix))), {});
    } else if (!trimmed.empty()) {
      throw std::invalid_argument("expected a 'MSG:' line, got [" + trimmed + "]");
    }
  }
  return sections;
}

}  // namespace

TypeDescriptionModel parseMsgDefinition(const std::string& rootTypeName, const std::string& definition) {
  std::map<std::string, IndividualTypeModel> types;
  for (const auto& section : splitSections(rootTypeName, definition)) {
    if (section.typeName.empty()) {
      continue;
    }
    IndividualTypeModel model;
    model.typeName = section.typeName;
    const std::string package = packageOf(section.typeName);
    for (const auto& line : section.lines) {
      const std::string content = trim(stripComment(line));
      if (!content.empty()) {
        parseLine(content, package, model);
      }
    }
    if (model.fields.empty()) {
      model.fields.push_back(FieldModel{kEmptyMessageFieldName, FieldTypeModel{field_type::kUint8, 0, 0, {}}, {}});
    }
    types.insert_or_assign(section.typeName, std::move(model));
  }

  std::set<std::string> reachable;
  std::vector<std::string> pending{rootTypeName};
  while (!pending.empty()) {
    const std::string typeName = pending.back();
    pending.pop_back();
    if (!reachable.insert(typeName).second) {
      continue;
    }
    const auto it = types.find(typeName);
    if (it == types.end()) {
      throw std::invalid_argument("definition of [" + typeName + "] is missing");
    }
    for (const auto& field : it->second.fields) {
      if (!field.type.nestedTypeName.empty()) {
        pending.push_back(field.type.nestedTypeName);
      }
    }
  }

  TypeDescriptionModel model;
  model.typeDescription = types.at(rootTypeName);
  for (const auto& typeName : reachable) {
    if (typeName != rootTypeName) {
      model.referencedTypeDescriptions.push_back(types.at(typeName));
    }
  }
  return model;
}

}  // namespace runtime_types

}  // namespace rqt_multiplot
