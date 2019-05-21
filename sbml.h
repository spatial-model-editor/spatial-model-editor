#ifndef SBML_H
#define SBML_H

#include <QStringList>
#include <QStringListModel>

#include <sbml/SBMLTypes.h>
#include <sbml/extension/SBMLDocumentPlugin.h>
#include <sbml/packages/spatial/extension/SpatialExtension.h>
#include <sbml/packages/spatial/common/SpatialExtensionTypes.h>

class sbmlDocWrapper {
private:
    std::unique_ptr<libsbml::SBMLDocument> doc;
    QStringList reactions;
    QStringList species;

public:
    QString xml;
    QStringListModel reac_model;
    QStringListModel spec_model;

    void loadFile(const std::string& filename);
};

#endif // SBML_H
