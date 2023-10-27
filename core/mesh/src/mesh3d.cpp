//
// Created by acaramizaru on 6/26/23.
//

#include "sme/mesh3d.hpp"

namespace sme::mesh {

// Mesh3d::Mesh3d(double facet_angle_in, double facet_size_in, double
// facet_distance_in,
//                double cell_radius_edge_ratio_in, double cell_size_in):
//                                                                         facet_angle_in(facet_angle_in),
//                                                                         facet_size_in(facet_size_in),
//                                                                         facet_distance_in(facet_distance_in),
//                                                                         cell_radius_edge_ratio_in(cell_radius_edge_ratio_in),
//                                                                         cell_size_in(cell_size_in)
//{}

Mesh3d::Mesh3d(const sme::model::Model &model, double facet_angle_in,
               double facet_size_in, double facet_distance_in,
               double cell_radius_edge_ratio_in, double cell_size_in)
    : facet_angle_in(facet_angle_in), facet_size_in(facet_size_in),
      facet_distance_in(facet_distance_in),
      cell_radius_edge_ratio_in(cell_radius_edge_ratio_in),
      cell_size_in(cell_size_in) {
  auto images = model.getGeometry().getImages();

  //  const unsigned char number_of_spheres = 50;
  //  const int max_radius_of_spheres = 10;
  //  const int radius_of_big_sphere = 80;
  _image *image_in = _createImage(
      images.volume().width(), images.volume().height(),
      images.volume().depth(), 1, 1.f, 1.f, 1.f, 1, WK_FIXED, SGN_UNSIGNED);
  unsigned char *ptr = static_cast<unsigned char *>(image_in->data);

  for (auto it = images.begin(); it != images.end(); it++) {
    auto image = *it;
    std::memcpy(ptr + (it - images.begin()) * images.volume().width() *
                          images.volume().height(),
                image.bits(),
                images.volume().width() * images.volume().height());
  }

  auto image = CGAL::Image_3(image_in);
  assert(image.is_valid());

  /// [Domain creation]
  Mesh_domain domain = Mesh_domain::create_labeled_image_mesh_domain(image);
  /// [Domain creation]

  // Mesh criteria
  Mesh_criteria criteria(facet_angle = facet_angle_in,
                         facet_size = facet_size_in,
                         facet_distance = facet_distance_in,
                         cell_radius_edge_ratio = cell_radius_edge_ratio_in,
                         cell_size = cell_size_in);

  /// [Meshing]
  c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria);
  /// [Meshing]

  /// [ build sub meshes ]
  sme::mesh::meshes_builder<C3t3>(submeshes, c3t3);
  /// [ sub meshes ]
}

void Mesh3d::Save(std::string fileName) {
  // Output
  std::ofstream medit_file(fileName);
  c3t3.output_to_medit(medit_file, true, false);
}

int Mesh3d::getNumberOfSubMeshes() { return submeshes.size(); }

const sme::mesh::SMesh &Mesh3d::GetMesh(int id) {
  assert(id >= 0 && id < submeshes.size());
  return submeshes[id];
}

} // namespace sme::mesh
