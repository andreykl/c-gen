#pragma once
#include <c-gen/source/graph.hpp>
#include <format>
#include <pugixml.hpp>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace cgen::source {

class GraphLoader {
public:
  static auto load_from_file(const std::string &filename) -> Graph {
    std::ifstream file(filename);
    if (!file.is_open()) {
      throw runtime_error(format("Could not open file: {}", filename));
    }
    return load_from_stream(file);
  }

  static auto load_from_stream(std::istream &stream) -> Graph {
    ParseContext ctx;
    pugi::xml_parse_result result = ctx.doc.load(stream);
    if (!result) {
      throw runtime_error(
          format("Failed to parse XML from stream: {}", result.description()));
    }
    parse_root(ctx);
    return std::move(ctx.graph);
  }

private:
  struct ParseContext {
    pugi::xml_document doc;
    Graph graph;
    string current_element;

    [[noreturn]] void throw_error(string message) const;
    [[noreturn]] void throw_error_with_block_info(string message, string name,
                                                  string SID) const;
  };

  std::vector<string> parse_array_portnames(const string &text) {
    vector<string> result;
    size_t start = text.find('[');
    size_t end = text.find(']');
    if (start == string::npos || end == string::npos || start >= end) {
      // something wrong
      throw runtime_error(format("Trying to parse array of portnames, but "
                                 "received '{}'. Interrupting.",
                                 text));
    }

    string inner =
        text.substr(start + 1, end - start - 1); // clean before [ and after ]
    stringstream ss(inner);
    string item;
    while (getline(ss, item, ',')) {
      result.push_back(item);
    }
    return result;
  }

  vector<int> parse_signs(const string &text) {
    vector<string> result;

    for (unsigned char ch : text) {
      if ('+' == ch) {
        result.push_back(1)
      } else if ('-' == ch) {
        result.push_back(-1)
      } else if (isspace(static_cast<unsigned char>(ch))) {
        // skip spaces
      } else {
        throw runtime_error(format("Trying to parse signs for inputs, but "
                                   "received '{}'. Interrupting.",
                                   text));
      }
    }

    return result;
  }

  static void parse_root(ParseContext &ctx) {
    auto root = ctx.doc.child("System");
    if (!root) {
      ctx.throw_error("Missing <System> root element");
    }

    for (auto block : root.children("Block")) {
      parse_block(ctx, block);
    }

    for (auto line : root.children("Line")) {
      parse_line(ctx, line);
    }
  }

  static void parse_block(ParseContext &ctx, const pugi::xml_node &block) {
    string type = block.attribute("BlockType").as_string();
    string name = block.attribute("Name").as_string();
    string sid = block.attribute("SID").as_string();

    if (name.empty()) {
      ctx.throw_error("Block missing 'Name' attribute");
    }

    if (sid.empty()) {
      ctx.throw_error("Block missing 'SID' attribute");
    }

    if ("Inport" == type) {
      ctx.graph.add_inport(name, sid);
    } else if ("Sum" == type) {
      vector<string> inputs = {"1"};
      vector<int> signs = {1};
      pugi::xpath_node xports = block.select_node("P[@Name='Ports']");
      if (xports) {
        inputs = parse_array_portnames(xporst.node().text().as_string());
      }
      pugi::xpath_node xsigns = block.select_node("P[@Name='Inputs']");
      if (xsigns) {
        signs = parse_signs(xsigns.node().text().as_string());
      }
      ctx.graph.add_sum(name, sid, inputs, signs);
    } else {
      ctx.throw_error_with_block_info(format("Unknown block type '{}'", type),
                                      name, sid);
    }
  }

  static void parse_line(ParseContext &ctx, const pugi::xml_node &xml_line);
};

} // namespace cgen::source
