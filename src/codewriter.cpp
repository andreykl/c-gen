// General guidlines:
// 1) The ';' character is emitted by the outer printing function, not by to_doc
// itself
// 2) Do not wrap the to_doc result in group() inside the function
// itself -- the outer function should do that if needed.

#include <c-gen/target/ast.hpp>
#include <c-gen/target/codewriter.hpp>

#include <prettyprint.h>

using namespace std;
using namespace pp;

namespace cgen::target {

/**
 * @brief Soft break -- either a new line or an empty string.
 */
auto inline sfbr() -> shared_ptr<doc> { return softbreak(); }

/**
 * @brief Space break -- either a new line or a space.
 */
auto inline spbr() -> shared_ptr<doc> { return line_or(" "); }

/**
 * @brief Just a space.
 */
auto inline space() -> shared_ptr<doc> { return text(" "); }

/**
 * @brief Join items into a document separated by @p sep.
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
 * @brief Overloaded version of separate() for a vector of docs.
 * @see separate
 */
auto inline separate(const shared_ptr<doc> &sep,
                     const vector<shared_ptr<doc>> &items) -> shared_ptr<doc> {
  return separate(sep, items, [](const auto &item) { return item; });
}

/**
 * @brief Forward declaration for the recursive Expr overload.
 */
static auto to_doc(const Expr &expr) -> std::shared_ptr<doc>;

/**
 * @brief Convert a File node to a document (expressions separated by blank
 * lines).
 */
static auto to_doc(const File &f) -> shared_ptr<doc> {
  return separate(line(), f.stmts, [](File::Statement const &e) {
    auto res = to_doc(e.expr);
    if (e.semicolon)
      res = res + text(";");
    if (e.linebreak)
      res = res + line();
    return res;
  });
}

/**
 * @brief Convert a StringLiteral to a quoted document.
 */
static auto to_doc(const StringLiteral &v) -> shared_ptr<doc> {
  return text("\"") + text(v.value) + text("\"");
}

/**
 * @brief Convert a RawLiteral to a document (verbatim text).
 */
static auto to_doc(const RawLiteral &v) -> shared_ptr<doc> {
  return text(v.value);
}

/**
 * @brief Unwrap a PPDoc node (already holds a document).
 */
static auto to_doc(const PPDoc &d) -> shared_ptr<doc> { return d.doc; }

/**
 * @brief Convert a StructDecl to "struct { type1 name1; type2 name2; ... }".
 */
static auto to_doc(const StructDecl &s) -> shared_ptr<doc> {
  auto fields = separate(line(), s.fields, [](const auto &f) {
    return to_doc(f.type) + space() + text(f.name) + text(";");
  });
  return text("struct") + line() + text("{") + nest(4, line() + fields) +
         line() + text("}");
}

/**
 * @brief Convert an Assignment to "lhs = rhs".
 */
static auto to_doc(const Assignment &a) -> shared_ptr<doc> {
  return group(to_doc(a.lhs) + spbr() + text("=")) +
         nest(4, spbr() + group(to_doc(a.rhs)));
}

/**
 * @brief Convert a VarDecl to a declaration with optional static/const
 * qualifiers.
 */
static auto to_doc(const VarDecl &v) -> shared_ptr<doc> {
  shared_ptr<doc> result = nil();
  if (v.is_static)
    result = result + text("static") + space();
  if (v.is_const)
    result = result + text("const") + space();
  return result + to_doc(v.type) + space() + text(v.name);
}

/**
 * @brief Convert a ListInit to a brace-enclosed, comma-separated element list.
 */
static auto to_doc(const ListInit &l) -> shared_ptr<doc> {
  auto elements = separate(text(",") + spbr(), l.elements,
                           [](const auto &e) { return group(to_doc(e)); });
  return text("{") + nest(4, spbr() + group(elements)) + spbr() + text("}");
}

/**
 * @brief Convert a FunDecl to 'type fun_name { body; }'
 */
static auto to_doc(const FunDecl &decl) -> shared_ptr<doc> {
  auto body = separate(text(";") + spbr(), decl.body,
                       [](const auto &e) { return group(to_doc(e)); });
  return group(to_doc(decl.type)) + space() + text(decl.name + "()") + space() +
         text("{") + nest(4, spbr() + group(body + text(";"))) + spbr() +
         text("}");
}

/**
 * @brief Visitor that dispatches to the appropriate to_doc overload.
 */
struct to_doc_visitor : public boost::static_visitor<shared_ptr<doc>> {
  template <typename T>
  auto operator()(const T &node) const -> shared_ptr<doc> {
    return to_doc(node);
  }
};

/**
 * @brief Convert any Expr variant to a document by dispatching via visitor.
 */
static auto to_doc(const Expr &expr) -> std::shared_ptr<doc> {
  return boost::apply_visitor(to_doc_visitor{}, expr);
}

CodeWriter::CodeWriter(std::ostream &out, int width)
    : out_(out), width_(width) {}

CodeWriter::~CodeWriter() = default;

/**
 * @brief Render the given expression to the output stream.
 */
void CodeWriter::write(const Expr &expr) {
  auto d = to_doc(expr);
  auto settings = std::cout << pp::set_width(width_);
  settings << d << std::endl;
}

} // namespace cgen::target
