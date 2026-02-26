// SBML xml annotation read/write

#pragma once

#include "sme/serialization.hpp"

namespace libsbml {
class Model;
}

namespace sme::model {

/**
 * @brief Write SME settings into model annotation XML.
 */
void setSbmlAnnotation(libsbml::Model *model, const Settings &sbmlAnnotation);
/**
 * @brief Read SME settings from model annotation XML.
 */
Settings getSbmlAnnotation(libsbml::Model *model);

} // namespace sme::model
