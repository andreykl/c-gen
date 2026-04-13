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
 * @brief Visitor that dispatches to the appropriate to_doc overload.
 */
struct to_doc_visitor {
  /**
   * @brief Convert a FunDecl to 'type fun_name { body; }'
   */
  auto operator()(const FunDecl &decl) const -> shared_ptr<doc> {
    auto body = separate(text(";") + spbr(), decl.body, [this](const auto &e) {
      return group(boost::apply_visitor(*this, e));
    });
    return group(boost::apply_visitor(*this, decl.type)) + space() +
           text(decl.name + "()") + space() + text("{") +
           nest(4, spbr() + group(body + text(";"))) + spbr() + text("}");
  }

  /**
   * @brief Convert a ListInit to a brace-enclosed, comma-separated element
   * list.
   */
  auto operator()(const ListInit &l) const -> shared_ptr<doc> {
    auto elements =
        separate(text(",") + spbr(), l.elements, [this](const auto &e) {
          return group(boost::apply_visitor(*this, e));
        });
    return text("{") + nest(4, spbr() + group(elements)) + spbr() + text("}");
  }

  /**
   * @brief Convert a VarDecl to a declaration with optional static/const
   * qualifiers.
   */
  auto operator()(const VarDecl &v) const -> shared_ptr<doc> {
    shared_ptr<doc> result = nil();
    if (v.is_static)
      result = result + text("static") + space();
    if (v.is_const)
      result = result + text("const") + space();
    return result + boost::apply_visitor(*this, v.type) + space() +
           text(v.name);
  }

  /**
   * @brief Convert a File node to a document (expressions separated by blank
   * lines).
   */
  auto operator()(const File &f) const -> shared_ptr<doc> {
    return separate(line(), f.stmts, [this](File::Statement const &e) {
      auto res = boost::apply_visitor(*this, e.expr);
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
  auto operator()(const StringLiteral &v) const -> shared_ptr<doc> {
    return text("\"") + text(v.value) + text("\"");
  }

  /**
   * @brief Convert a RawLiteral to a document (verbatim text).
   */
  auto operator()(const RawLiteral &v) const -> shared_ptr<doc> {
    return text(v.value);
  }

  /**
   * @brief Unwrap a PPDoc node (already holds a document).
   */
  auto operator()(const PPDoc &d) const -> shared_ptr<doc> { return d.doc; }

  /**
   * @brief Convert a StructDecl to "struct { type1 name1; type2 name2; ...
   }".
   */
  auto operator()(const StructDecl &s) const -> shared_ptr<doc> {
    auto fields = separate(line(), s.fields, [this](const auto &f) {
      return boost::apply_visitor(*this, f.type) + space() + text(f.name) +
             text(";");
    });
    return text("struct") + line() + text("{") + nest(4, line() + fields) +
           line() + text("}");
  }

  /**
   * @brief Convert an Assignment to "lhs = rhs".
   */
  auto operator()(const Assignment &a) const -> shared_ptr<doc> {
    return group(boost::apply_visitor(*this, a.lhs) + spbr() + text("=")) +
           nest(4, spbr() + group(boost::apply_visitor(*this, a.rhs)));
  }
};

CodeWriter::CodeWriter(std::ostream &out, int width)
    : out_(out), width_(width) {}

CodeWriter::~CodeWriter() = default;

/**
 * @brief Render the given expression to the output stream.
 */
void CodeWriter::write(const Expr &expr) {
  auto d = boost::apply_visitor(to_doc_visitor{}, expr);
  auto settings = std::cout << pp::set_width(width_);
  settings << d << std::endl;
}

} // namespace cgen::target
