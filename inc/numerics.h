#ifndef NUMERICS_H
#define NUMERICS_H

#include <string>

// disable unused features: reduces build time and executable size
#define exprtk_disable_break_continue
#define exprtk_disable_sc_andor
#define exprtk_disable_return_statement
#define exprtk_disable_enhanced_features
#define exprtk_disable_string_capabilities
#define exprtk_disable_superscalar_unroll
#define exprtk_disable_rtl_io_file
#define exprtk_disable_rtl_vecops
#define exprtk_disable_caseinsensitivity
#include "exprtk.hpp"

namespace numerics {

class reaction_eval {
private:
  exprtk::symbol_table<double> symbol_table;
  exprtk::expression<double> expression;

public:
  // compile the given string containing a mathematical expression using exprtk
  // the species are stored as references, so their values can be changed
  // and the expression can then be re-evaluated without re-compiling
  reaction_eval(const std::string &expression_string,
                const std::vector<std::string> &species_name,
                std::vector<double> &species_value,
                const std::vector<std::string> &constant_name,
                const std::vector<double> &constant_value);
  // evaluate compiled expression
  double operator()();
};

} // namespace numerics

#endif // NUMERICS_H
