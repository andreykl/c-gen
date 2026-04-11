#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>

#include <prettyprint.h>
#include <variant>

using namespace std;
using namespace pp;

namespace cgen::target {

/**
 * @brief soft break -- either a new line or an empty string
 */
auto inline sfbr() -> shared_ptr<doc> { return softbreak(); }

/**
 * @brief space break -- either a new line or a space
 */
auto inline spbr() -> shared_ptr<doc> { return line_or(" "); }

/**
 * @brief just a space
 */
auto inline space() -> shared_ptr<doc> { return text(" "); }

/**
 * @brief I.e. to print array with separator with arbitrary type T.
 * @tparam T Type of elements in the vector.
 * @tparam F Callable that converts T to shared_ptr<doc>.
 * @param sep Separator document placed between elements.
 * @param items Vector of items to be converted and joined.
 * @param transform Function to convert each item to a document.
 * @return Combined document: transform(items[0]) + sep + transform(items[1]) +
 * sep ...
 */
template <typename T, typename F>
auto separate(const shared_ptr<doc> &sep, const vector<T> &items,
              F transform) -> shared_ptr<doc> {
  if (items.empty())
    return nil();

  auto it = items.begin();
  auto result = transform(*it);
  for (++it; it != items.end(); ++it) {
    result = result + sep + transform(*it);
  }
  return result;
}

/**
 * @brief overloaded version to print array with doc
 * @see separate
 */
auto inline separate(const shared_ptr<doc> &sep,
                     const vector<shared_ptr<doc>> &items) -> shared_ptr<doc> {
  return separate(sep, items, [](const auto &item) { return item; });
}

static auto to_doc(const Expr &expr) -> std::shared_ptr<doc>;

static auto to_doc(const File &f) -> shared_ptr<doc> {
  return separate(line() + line(), f.exprs,
                  [](auto const &e) { return to_doc(e); });
}

static auto to_doc(const StringLiteral &v) -> shared_ptr<doc> {
  return text("\"") + text(v.value) + text("\"");
}

static auto to_doc(const RawLiteral &v) -> shared_ptr<doc> {
  return text(v.value);
}

static auto to_doc(const PPDoc &d) -> shared_ptr<doc> { return d.doc; }

static auto to_doc(const StructInit &s) -> shared_ptr<doc> {
  return group(text("{") + spbr() +
               nest(4, separate(text(",") + spbr(), s.fields,
                                [](const auto &e) { return to_doc(e); }) +
                           spbr() + text("}")));
}

static auto to_doc(const ArrayInit &a) -> shared_ptr<doc> {
  shared_ptr<doc> prefix = nil();
  if (a.is_static)
    prefix = prefix + text("static") + spbr();
  if (a.is_const)
    prefix = prefix + text("const") + spbr();

  auto elements = separate(text(",") + spbr(), a.elements,
                           [](const auto &e) { return to_doc(e); });
  return group(group(prefix + text(a.type_name) + spbr() +
                     text(a.variable_name) + text("[] =")) +
               line() + text("{") + nest(4, line() + elements) + spbr() +
               text("};"));
}

static auto to_doc(const PStruct &p) -> shared_ptr<doc> {
  shared_ptr<doc> prefix = nil();
  if (p.is_static)
    prefix = prefix + text("static") + space();
  prefix = prefix + text("struct") + line() + "{";

  return prefix +
         nest(4, group(line() +
                       separate(line(), p.fields,
                                [](const auto &e) { return to_doc(e); }))) +
         line() + text("} " + p.variable_name + ";");
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
