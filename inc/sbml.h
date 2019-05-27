#ifndef SBML_H
#define SBML_H

#include <QColor>
#include <QDebug>
#include <QStringList>
#include <QStringListModel>

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>

class sbmlDocWrapper {
private:
  std::unique_ptr<libsbml::SBMLDocument> doc;

public:
  libsbml::Model *model = nullptr;

  QString xml;

  // loop up given specied ID, return index
  std::map<std::string, std::size_t> speciesIndex;
  // vector of speciesID with above indexing
  std::vector<std::string> speciesID;

  QStringList reactions;
  // list of species for given compartment ID
  std::map<QString, QStringList> species;
  QStringList compartments;

  // colour of compartment of given index in image
  std::map<QString, QRgb> compartment_colour;

  // reactions contribution to PDE for given species, as infix string
  std::map<QString, QString> pde;

  void loadFile(const std::string &filename);
};

#endif // SBML_H

// NOTE:
// SBML uses internal AST to represent maths
// but mathML in the SBML file
// to convert mathML to AST:
// ASTNode* ast    = readMathMLFromString(s);
// to convert AST to infix:
// char*    result = SBML_formulaToL3String(ast);
