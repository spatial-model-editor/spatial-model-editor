#include <unordered_map>

#include "model.h"

void Geometry::init(QImage img, QRgb col) {
  img_size = img.size();
  img_comp = QImage(img_size, QImage::Format_Mono);
  img_comp.setColor(0, qRgba(0, 0, 0, 0));
  img_comp.setColor(1, qRgba(0, 0, 0, 255));
  img_comp.fill(0);
  ix.clear();
  // find pixels in compartment: store image QPoint for each
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      if (img.pixel(x, y) == col) {
        // if colour matches, add pixel to field
        // qDebug("%d, %d", x, y);
        QPoint p = QPoint(x, y);
        ix.push_back(p);
        img_comp.setPixel(p, 1);
      }
    }
  }
}

const QImage &Geometry::getCompartmentImage() { return img_comp; }

void Field::init(Geometry *geom, const std::vector<std::string> &speciesIDvec,
                 BOUNDARY_CONDITION bc) {
  geometry = geom;
  speciesID = speciesIDvec;
  n_species = speciesID.size();
  mapSpeciesIdToIndex.clear();
  for (std::size_t i = 0; i < speciesID.size(); ++i) {
    mapSpeciesIdToIndex[speciesID[i]] = i;
  }
  n_pixels = geom->ix.size();
  qDebug("field: n_species: %lu", n_species);
  img_conc = QImage(geom->img_size, QImage::Format_ARGB32);

  // set diffusion constants to 1 for now:
  diffusion_constant = std::vector<double>(n_species, 1.0);
  std::unordered_map<int, std::size_t> index;
  // temporary map from (x,y) point to ix index for neighbour lookup
  std::size_t i = 0;
  for (const auto &p : geometry->ix) {
    index[p.x() * geometry->img_size.height() + p.y()] = i++;
  }

  nn.clear();
  nn.reserve(4 * geometry->ix.size());
  // find neighbours of each pixel in compartment
  std::size_t outside = geometry->ix.size();
  for (const auto &p : geometry->ix) {
    for (const auto &pp :
         {QPoint(p.x() + 1, p.y()), QPoint(p.x() - 1, p.y()),
          QPoint(p.x(), p.y() + 1), QPoint(p.x(), p.y() - 1)}) {
      auto it = index.find(pp.x() * geometry->img_size.height() + pp.y());
      if (it != index.cend()) {
        // qDebug() << pp;
        nn.push_back(it->second);
      } else {
        if (bc == DIRICHLET) {
          // Dirichlet bcs: specify value of conc. at boundary.
          // Here all points on boundary point to the same pixel with
          // index, "outside", which will typically have zero concentration
          nn.push_back(outside);
        } else if (bc == NEUMANN) {
          // Neumann bcs: specify derivative of conc. in direction normal to
          // boundary. Here we define a zero flux condition by setting the value
          // of the boundary conc. to be equal to that of the neighbour.
          // This is done here very naively by making the neighbour of the pixel
          // point to the pixel itself.
          nn.push_back(index.at(p.x() * geometry->img_size.height() + p.y()));
        } else {
          qDebug() << "Error: boundary condition not supported";
          exit(1);
        }
      }
    }
  }
  // add n_species per pixel in comp, plus one for each boundary value
  conc.resize(n_species * (geometry->ix.size() + n_bcs), 0.0);
  dcdt = conc;
}

void Field::importConcentration(std::size_t species_index, QImage img,
                                double scale_factor) {
  // rescaling [min, max] pixel values to the range [0, scale_factor] for now
  QRgb min = std::numeric_limits<QRgb>::max();
  QRgb max = 0;
  for (int x = 0; x < img.width(); ++x) {
    for (int y = 0; y < img.height(); ++y) {
      min = std::min(min, img.pixel(x, y));
      max = std::max(max, img.pixel(x, y));
    }
  }
  for (std::size_t i = 0; i < geometry->ix.size(); ++i) {
    conc[n_species * i + species_index] =
        scale_factor * static_cast<double>(img.pixel(geometry->ix[i]) - min) /
        static_cast<double>(max - min);
  }
}

void Field::importConcentration(const std::string &species_ID, QImage img,
                                double scale_factor) {
  importConcentration(mapSpeciesIdToIndex.at(species_ID), img, scale_factor);
}

void Field::setConstantConcentration(std::size_t species_index,
                                     double concentration) {
  for (std::size_t i = 0; i < geometry->ix.size(); ++i) {
    conc[n_species * i + species_index] = concentration;
  }
}

const QImage &Field::getConcentrationImage(
    const std::vector<std::size_t> &species_indices) {
  img_conc.fill(qRgba(0, 0, 0, 0));
  // alpha opacity factor
  double alpha = 0.75;
  // for now rescale each conc to [0,1] to multiply species colour
  std::vector<double> max_conc(species_indices.size(), 1e-5);
  for (std::size_t i = 0; i < geometry->ix.size(); ++i) {
    for (const auto species_index : species_indices) {
      max_conc[species_index] = std::max(max_conc[species_index],
                                         conc[i * n_species + species_index]);
    }
  }
  for (std::size_t i = 0; i < geometry->ix.size(); ++i) {
    int r = 0;
    int g = 0;
    int b = 0;
    for (const auto species_index : species_indices) {
      double c = conc[i * n_species + species_index] / max_conc[species_index];
      r += static_cast<int>((speciesColour[species_index].red() * c) * alpha);
      g += static_cast<int>((speciesColour[species_index].green() * c) * alpha);
      b += static_cast<int>((speciesColour[species_index].blue() * c) * alpha);
      img_conc.setPixel(
          geometry->ix[i],
          QColor(r > 255 ? 255 : r, g > 255 ? 255 : g, b > 255 ? 255 : b, 255)
              .rgba());
    }
  }
  return img_conc;
}

const QImage &Field::getConcentrationImage(std::string species_ID) {
  return getConcentrationImage(
      std::vector<std::size_t>{mapSpeciesIdToIndex.at(species_ID)});
}

const QImage &Field::getConcentrationImage(std::size_t species_index) {
  return getConcentrationImage(std::vector<std::size_t>{species_index});
}

const QImage &Field::getConcentrationImage() {
  std::vector<std::size_t> indices(n_species);
  for (std::size_t i = 0; i < n_species; ++i) {
    indices[i] = i;
  }
  return getConcentrationImage(indices);
}

void Field::applyDiffusionOperator() {
  for (std::size_t i = 0; i < geometry->ix.size(); ++i) {
    std::size_t index = n_species * i;
    std::size_t xup = n_species * nn[4 * i];
    std::size_t xdn = n_species * nn[4 * i + 1];
    std::size_t yup = n_species * nn[4 * i + 2];
    std::size_t ydn = n_species * nn[4 * i + 3];
    for (std::size_t s = 0; s < n_species; ++s) {
      dcdt[index + s] = diffusion_constant[s] *
                        (conc[xup + s] + conc[xdn + s] + conc[yup + s] +
                         conc[ydn + s] - 4 * conc[index + s]);
    }
  }
}

double Field::getMeanConcentration(std::size_t species_index) {
  double sum = 0;
  for (std::size_t i = 0; i < geometry->ix.size(); ++i) {
    sum += conc[n_species * i + species_index];
  }
  return sum / static_cast<double>(geometry->ix.size());
}
