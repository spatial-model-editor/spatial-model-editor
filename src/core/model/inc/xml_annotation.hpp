// SBML xml annotation read/write

#pragma once

#include "serialization.hpp"

namespace libsbml {
class Model;
}

namespace sme::model {

void setSbmlAnnotation(libsbml::Model *model,
                       const Settings &sbmlAnnotation);
Settings getSbmlAnnotation(libsbml::Model *model);

} // namespace sme::model
