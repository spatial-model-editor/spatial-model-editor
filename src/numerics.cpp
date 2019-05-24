#include "numerics.h"

namespace numerics{

reaction_eval::reaction_eval(const std::string& expression_string, const std::vector<std::string>& species_name, std::vector<double>& species_value, const std::vector<std::string>& constant_name, const std::vector<double>& constant_value){
    // compile the given string containing a mathematical expression using exprtk
    // the species are stored as references, so their values can be changed
    // and the expression can then be re-evaluated without re-compiling
    for(std::size_t i=0; i<species_name.size(); ++i){
        symbol_table.add_variable(species_name[i], species_value[i]);
    }
    for(std::size_t i=0; i<constant_name.size(); ++i){
        symbol_table.add_constant(constant_name[i], constant_value[i]);
    }
    expression.register_symbol_table(symbol_table);
    parser.compile(expression_string, expression);
}

double reaction_eval::operator()(){
    return expression.value();
}

}
