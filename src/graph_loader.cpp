#include <c-gen/source/graph.hpp>
#include <c-gen/source/graph_loader.hpp>
#include <format>
#include <fstream>
#include <ranges>
#include <sstream>

namespace cgen::source {
using namespace std;

auto GraphLoader::load_from_file(const string &filename) -> Graph {
  ifstream file(filename);
  if (!file.is_open()) {
    throw runtime_error(format("Could not open file: {}", filename));
  }
  return load_from_stream(file);
}

auto GraphLoader::load_from_stream(std::istream &stream) -> Graph {
  ParseContext ctx;
  pugi::xml_parse_result result = ctx.doc.load(stream);
  if (!result) {
    throw runtime_error(
        format("Failed to parse XML from stream: {}", result.description()));
  }
  parse_root(ctx);
  return std::move(ctx.graph);
}

[[noreturn]] void GraphLoader::ParseContext::throw_error(string message) const {
  throw runtime_error(
      format("Error during building a graph. {}. Interrupting.", message));
}
[[noreturn]] void GraphLoader::ParseContext::throw_error_with_block_info(
    string message, string name, string SID) const {
  throw_error(format("{}. Node info name '{}', SID '{}'.", message, name, SID));
}

static auto parse_array_portnames(const string &text) -> vector<string> {
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

auto GraphLoader::parse_signs(const string &text) -> vector<int> {
  vector<int> result;

  for (unsigned char ch : text) {
    if ('+' == ch) {
      result.push_back(1);
    } else if ('-' == ch) {
      result.push_back(-1);
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

auto GraphLoader::parse_line_value(const string s) -> pair<string, int> {
  stringstream val(s);
  string sid, type, port;
  if (getline(val, sid, '#')) {
    if (getline(val, type, ':')) {
      if (getline(val, port)) {
        return {boost::trim_copy(sid), std::stoi(port)};
      }
      throw runtime_error(
          "parse_line_value: wrong value format, no port after ':' found");
    } else {
      throw runtime_error("parse_line_value: wrong value format, no ':' "
                          "found or no type between '#' and ':'");
    }
  } else {
    throw runtime_error("parse_line_value: wrong value format, no '#' found "
                        "or no sid before it");
  }
}

void GraphLoader::parse_root(GraphLoader::ParseContext &ctx) {
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

void GraphLoader::parse_block(GraphLoader::ParseContext &ctx,
                              const pugi::xml_node &block) {
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
    vector<string> inputs = {};
    vector<int> signs = {};
    pugi::xpath_node xports = block.select_node("P[@Name='Ports']");
    if (xports) {
      inputs = parse_array_portnames(xports.node().text().as_string());
    }
    pugi::xpath_node xsigns = block.select_node("P[@Name='Inputs']");
    if (xsigns) {
      signs = parse_signs(xsigns.node().text().as_string());
    }
    ctx.graph.add_sum(name, sid, inputs, signs);
  } else if ("Gain" == type) {
    double gain = 0;
    pugi::xpath_node xgain = block.select_node("P[@Name='Gain']");
    if (!xgain) {
      ctx.throw_error_with_block_info("Gain does not contain gain information.",
                                      name, sid);
    }
    gain = xgain.node().text().as_double();
    ctx.graph.add_gain(name, sid, gain);
  } else if ("UnitDelay" == type) {
    int sample_time = -1;
    pugi::xpath_node xsample = block.select_node("P[@Name='SampleTime']");
    if (!xsample) {
      ctx.throw_error_with_block_info(
          "UnitDelay does not contain sample time information.", name, sid);
    }
    sample_time = xsample.node().text().as_int();
    ctx.graph.add_unit_delay(name, sid, sample_time);
  } else if ("Outport" == type) {
    ctx.graph.add_outport(name, sid);
  } else {
    ctx.throw_error_with_block_info(format("Unknown block type '{}'", type),
                                    name, sid);
  }
}

void GraphLoader::parse_line(GraphLoader::ParseContext &ctx,
                             const pugi::xml_node &line) {
  pugi::xpath_node xnode = line.select_node("P[@Name='Src']");
  if (!xnode) {
    ctx.throw_error("line without Src");
  }
  auto [srcSID, _] = parse_line_value(xnode.node().text().as_string());

  auto ps = line.select_nodes(".//P[@Name='Dst']");
  bool found = false;
  for (auto &p : ps) {
    auto [dstSID, dst_port] = parse_line_value(p.node().text().as_string());
    found = true;
    ctx.graph.add_line(srcSID, dstSID, dst_port - 1);
  }

  if (!found) {
    ctx.throw_error("No P node with attribute Name='Dst' found for line");
  }
}
} // namespace cgen::source
