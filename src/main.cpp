#include <c-gen/source/graph.hpp>
#include <c-gen/source/graph_loader.hpp>
#include <c-gen/target/codewriter.hpp>
#include <iostream>

const int DOC_WIDTH = 80;

using namespace cgen::target;
using namespace cgen::source;

auto main(int argc, char *argv[]) -> int {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <filename.xml>" << std::endl;
    return 1;
  }

  string filename = argv[1];
  auto g = GraphLoader::load_from_file(filename);
  
  g.transform(CodeWriter(std::cout, DOC_WIDTH));

  return 0;
}
