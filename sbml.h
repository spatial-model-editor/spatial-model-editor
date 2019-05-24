#ifndef SBML_H
#define SBML_H

#include <QStringList>
#include <QStringListModel>
#include <QDebug>
#include <QColor>

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>

class sbmlDocWrapper {
private:

public:
    std::unique_ptr<libsbml::SBMLDocument> doc;

    QString xml;

    QStringList reactions;
    // list of species for given compartment ID
    std::map<QString, QStringList> species;
    QStringList compartments;

    // colour of compartment of given index in image
    std::map<QString, QRgb> compartment_colour;

    void loadFile(const std::string& filename);
};

#endif // SBML_H

// NOTE:
// SBML uses internal AST to represent maths
// but mathML in the SBML file
// to convert mathML to AST:
// ASTNode* ast    = readMathMLFromString(s);
// to convert AST to infix:
// char*    result = SBML_formulaToL3String(ast);
