#include <iostream>

#include <boost/spirit/home/x3.hpp>
#include "expression_def.hpp"
#include "printer.hpp"

int main(int argc, char ** argv)
{
  namespace x3 = boost::spirit::x3;

  if (argc < 2)
    return -1;

  std::string s(argv[1] ? argv[1] : "");
  auto it = s.begin();
  auto end = s.end();

  using iterator_type = std::string::const_iterator;
  using position_cache = boost::spirit::x3::position_cache<std::vector<iterator_type>>;

  position_cache positions{s.begin(), s.end()};

  lspredicate::ast::expression expr;
  auto const parser =
    x3::with<lspredicate::parser::position_cache_tag>(std::ref(positions))[lspredicate::parser::expression_def];

  bool r = x3::phrase_parse(it, end, parser, x3::ascii::space, expr);

  if (r && it == end)
    {
        std::cout << "-------------------------\n";
        std::cout << "Parsing succeeded\n";
        std::cout << "-------------------------\n";
	lspredicate::ast::print(std::cout, expr);
    }
    else
    {
        std::string context(it, end);
        std::cout << "-------------------------\n";
        std::cout << "Parsing failed\n";
        std::cout << "stopped at: \"" << context << "...\"\n";
        std::cout << "-------------------------\n";
        return 1;
    }

  return 0;
}
