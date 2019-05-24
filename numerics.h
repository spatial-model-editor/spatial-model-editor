#ifndef NUMERICS_H
#define NUMERICS_H

#include <string>

#include "exprtk.hpp"

namespace numerics{

class reaction_eval {
private:
    exprtk::symbol_table<double> symbol_table;
    exprtk::expression<double> expression;
    exprtk::parser<double> parser;
public:
    reaction_eval(const std::string& expression_string, const std::vector<std::string>& species_name, std::vector<double>& species_value, const std::vector<std::string>& constant_name, const std::vector<double>& constant_value);
    double operator()();
};

}

#endif // NUMERICS_H
