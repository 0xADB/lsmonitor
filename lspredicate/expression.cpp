#include "expression_def.hpp"
#include <string>

namespace lspredicate
{
  namespace x3 = boost::spirit::x3;

  using iterator_type = std::string::const_iterator;
  using phrase_context_type = x3::phrase_parse_context<x3::ascii::space_type>::type;
  using position_cache = boost::spirit::x3::position_cache<std::vector<iterator_type>>;
  using error_handler_type = error_handler<iterator_type>;
  using context_type = x3::with_context<
    error_handler_tag
    , std::reference_wrapper<error_handler_type> const
    , phrase_context_type
    >::type;

  namespace parser
  {
    BOOST_SPIRIT_INSTANTIATE(expression_type, iterator_type, context_type);
  }
} // lspredicate
