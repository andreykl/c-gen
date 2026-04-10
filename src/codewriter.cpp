#include "c-gen/target/codewriter.hpp"
#include "c-gen/target/ast.hpp"
// #include <functional>
#include <prettyprint.h>
#include <variant>

using namespace std;
using namespace pp;

namespace cgen::target {

// using namespace prettyprint;

// Helper for visiting variants
template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

auto space() -> shared_ptr<doc> { return text(" "); }

template <typename T, typename F>
/* auto separate(const shared_ptr<doc> &sep, const vector<T> &items,
              const function<shared_ptr<doc>(const T &)> &transform)
    -> shared_ptr<doc> { */
auto separate(const shared_ptr<doc> &sep, const vector<const T> &items,
              F transform) -> shared_ptr<doc> {
  shared_ptr<doc> result = nil();
  for (size_t i = 0; i < items.size(); ++i) {
    const shared_ptr<doc> t = transform(items[i]);
    result = result + t;
    if (i < items.size() - 1) {
      result = result + sep;
    }
  }
  return result;
}

/**
 * @brief Recursive function to convert AST nodes to PrettyPrint Documents.
 */
auto toDoc(const Expr &expr) -> shared_ptr<doc> {
  return visit(
      overloaded{// 1. String literal: adds quotes around the value
                 [](const File &f) {
                   return text("\"") + text("this is file") + text("\"");
                 },
                 [](const StringLiteral &v) {
                   return text("\"") + text(v.value) + text("\"");
                 },

                 // 2. Raw literal: prints value as is (numbers, pointers, etc.)
                 [](const RawLiteral &v) { return text(v.value); },

                 // 3. Struct initializer: { a, b, c }
                 [](const StructInit &s) {
                   vector<shared_ptr<doc>> fields;
                   for (const auto &f : s.fields) {
                     fields.push_back(toDoc(f));
                   }
                   return group(text("{") + space() +
                                nest(2, text("hellooooooo") + space()) +
                                space() + text("}"));

                   // group() decides if it's one line or multi-line
                   /*return group(
                       text("{") + space() +
                       nest(2, separate(text(",") + space(), fields,
                                        [](const Expr &e) -> shared_ptr<doc> {
                                          return toDoc(e);
                                        })) +
                       space() + text("}"));
                        */
                 },

                 // 4. Array of structs: static const Type Name[] = { ... };
                 [](const ArrayInit &a) {
                   vector<shared_ptr<doc>> rows;
                   for (const auto &r : a.elements) {
                     // Convert each StructInit to doc
                     rows.push_back(toDoc(Expr(r)));
                   }

                   shared_ptr<doc> prefix = nil();
                   if (a.is_static)
                     prefix = prefix + text("static") + space();
                   if (a.is_const)
                     prefix = prefix + text("const") + space();

                   return group(prefix + text(a.type_name) + space() +
                                text(a.variable_name) + text("[] =") + line() +
                                text("{") +
                                nest(4, line() + text("worlddddddddddd") +
                                            line() + text("}")));
                   // nest(4, line() +
                   //             separate(text(",") + line(), rows,
                   //                      [](const Expr &e) { return toDoc(e);
                   //                      })) +
                   // line() + text("};"));
                 }},
      expr);
}

CodeWriter::CodeWriter(std::ostream &out, int width)
    : out_(out), width_(width) {}

CodeWriter::~CodeWriter() = default;

void CodeWriter::write(const Expr &expr) {
  auto d = toDoc(expr);
  auto settings = std::cout << pp::set_width(width_);
  settings << d << std::endl;
}

} // namespace cgen::target
