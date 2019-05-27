#include "numerics.h"

namespace numerics{

reaction_eval::reaction_eval(const std::string& expression_string, const std::vector<std::string>& species_name, std::vector<double>& species_value, const std::vector<std::string>& constant_name, const std::vector<double>& constant_value){
    for(std::size_t i=0; i<species_name.size(); ++i){
        symbol_table.add_variable(species_name[i], species_value[i]);
    }
    for(std::size_t i=0; i<constant_name.size(); ++i){
        symbol_table.add_constant(constant_name[i], constant_value[i]);
    }
    expression.register_symbol_table(symbol_table);
    exprtk::parser<double> parser;
    parser.compile(expression_string, expression);
}

double reaction_eval::operator()(){
    return expression.value();
}

}
