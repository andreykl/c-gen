#pragma once
#include <boost/algorithm/string.hpp>
#include <c-gen/source/graph.hpp>
#include <pugixml.hpp>
#include <string>
#include <vector>

using namespace std;

namespace cgen::source {

class GraphLoader {
public:
  static auto load_from_file(const string &filename) -> Graph;
  static auto load_from_stream(std::istream &stream) -> Graph;

private:
  struct ParseContext {
    pugi::xml_document doc;
    Graph graph;
    string current_element;

    [[noreturn]] void throw_error(string message) const;
    [[noreturn]] void throw_error_with_block_info(string message, string name,
                                                  string SID) const;
  };
  static auto parse_signs(const string &text) -> vector<int>;
  static auto parse_line_value(const string s) -> pair<string, int>;
  static void parse_root(ParseContext &ctx);
  static void parse_block(ParseContext &ctx, const pugi::xml_node &block);
  static void parse_line(ParseContext &ctx, const pugi::xml_node &line);
};

} // namespace cgen::source
