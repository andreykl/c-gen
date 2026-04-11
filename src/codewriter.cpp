#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>

#include <prettyprint.h>
#include <variant>

using namespace std;
using namespace pp;

namespace cgen::target {

// soft break -- either a new line or an empty string
auto inline sfbr() -> shared_ptr<doc> { return softbreak(); }
// space break -- either a new line or a space
auto inline spbr() -> shared_ptr<doc> { return line_or(" "); }
// just a space
auto inline space() -> shared_ptr<doc> { return text(" "); }

// to print array in example
auto separate(const shared_ptr<doc> &sep,
              const vector<shared_ptr<doc>> &items) -> shared_ptr<doc> {
  shared_ptr<doc> result = nil();
  for (size_t i = 0; i < items.size(); ++i) {
    result = result + items[i];
    if (i < items.size() - 1) {
      result = result + sep;
    }
  }
  return result;
}

static auto to_doc(const Expr &expr) -> std::shared_ptr<doc>;

static auto to_doc(const File &f) -> shared_ptr<doc> {
  return text("\"") + text("this is file") + text("\"");
}

static auto to_doc(const StringLiteral &v) -> shared_ptr<doc> {
  return text("\"") + text(v.value) + text("\"");
}

static auto to_doc(const RawLiteral &v) -> shared_ptr<doc> {
  return text(v.value);
}

static auto to_doc(const StructInit &s) -> shared_ptr<doc> {
  vector<shared_ptr<doc>> fields;
  for (const auto &f : s.fields) {
    fields.push_back(to_doc(f));
  }
  // group() decides if it's one line or multi-line
  return group(
      text("{") + spbr() +
      nest(4, separate(text(",") + spbr(), fields) + spbr() + text("}")));
}

static auto to_doc(const ArrayInit &a) -> shared_ptr<doc> {
  vector<shared_ptr<doc>> rows;
  for (const auto &r : a.elements) {
    rows.push_back(to_doc(r));
  }

  shared_ptr<doc> prefix = nil();
  if (a.is_static)
    prefix = prefix + text("static") + spbr();
  if (a.is_const)
    prefix = prefix + text("const") + spbr();

  return group(
      group(prefix + text(a.type_name) + spbr() + text(a.variable_name) +
            text("[] =") + spbr()) +
      nest(4, text("{") + nest(4, spbr() + separate(text(",") + spbr(), rows)) +
                  spbr() + text("};")));
}

/**
 * @brief Recursive function to convert AST nodes to PrettyPrint Documents.
 */
static auto to_doc(const Expr &expr) -> std::shared_ptr<doc> {
  return std::visit(
      [](const auto &concrete_node) { return to_doc(concrete_node); }, expr);
}

CodeWriter::CodeWriter(std::ostream &out, int width)
    : out_(out), width_(width) {}

CodeWriter::~CodeWriter() = default;

void CodeWriter::write(const Expr &expr) {
  auto d = to_doc(expr);
  auto settings = std::cout << pp::set_width(width_);
  settings << d << std::endl;
}

} // namespace cgen::target
