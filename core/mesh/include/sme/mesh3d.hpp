//
// Created by acaramizaru on 6/26/23.
//

#ifndef SPATIALMODELEDITOR_MESH3D_HPP
#define SPATIALMODELEDITOR_MESH3D_HPP

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Surface_mesh/Surface_mesh.h>

#include "sme/utils.hpp"

#include "sme/image_stack.hpp"
#include "sme/model.hpp"

#include <CGAL/Simple_cartesian.h>
#include <vector>

#include <CGAL/IO/File_medit.h>

//***********

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>
#include <CGAL/Mesh_triangulation_3.h>

#include <CGAL/Image_3.h>
#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/make_mesh_3.h>

//***********

namespace sme::mesh {

// Domain
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Labeled_mesh_domain_3<K> Mesh_domain;

#ifdef CGAL_CONCURRENT_MESH_3
typedef CGAL::Parallel_tag Concurrency_tag;
#else
typedef CGAL::Sequential_tag Concurrency_tag;
#endif

// Triangulation
typedef CGAL::Mesh_triangulation_3<Mesh_domain, CGAL::Default,
                                   Concurrency_tag>::type Tr;

typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr> C3t3;

// Criteria
typedef CGAL::Mesh_criteria_3<Tr> Mesh_criteria;

// To avoid verbose function and named parameters call
using namespace CGAL::parameters;

// ********

typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_3 Point;

typedef CGAL::Surface_mesh<Point> SMesh;

typedef CGAL::Simple_cartesian<double> kernel;
typedef kernel::Point_3 Point_3;
typedef sme::mesh::SMesh::Vertex_index Index;

template <class C3T3, class Vertex_index_property_map,
          class Facet_index_property_map, class Facet_index_property_map_twice,
          class Cell_index_property_map>
void meshes_builder(std::vector<sme::mesh::SMesh> &submeshes, const C3T3 &c3t3,
                    const Vertex_index_property_map &vertex_pmap,
                    const Facet_index_property_map &facet_pmap,
                    const Cell_index_property_map &cell_pmap,
                    const Facet_index_property_map_twice &facet_twice_pmap,
                    const bool print_each_facet_twice) {
  typedef typename C3T3::Triangulation Tr;
  typedef typename C3T3::Facets_in_complex_iterator Facet_iterator;
  typedef typename C3T3::Cells_in_complex_iterator Cell_iterator;

  typedef typename Tr::Finite_vertices_iterator Finite_vertices_iterator;
  typedef typename Tr::Vertex_handle Vertex_handle;
  typedef typename Tr::Point Point; // can be weighted or not

  //  int number_of_meshes = cell_pmap.subdomain_number();
  int number_of_meshes = 0;

  for (Facet_iterator fit = c3t3.facets_in_complex_begin();
       fit != c3t3.facets_in_complex_end(); ++fit) {
    number_of_meshes = std::max(get(facet_pmap, *fit) + 1, number_of_meshes);
  }

  std::vector<int> meshes_vertex_reindexation(number_of_meshes, 0);
  submeshes.resize(number_of_meshes);
  const Tr &tr = c3t3.triangulation();

  std::unordered_map<Vertex_handle, int> Vertex_reindexed;

  for (Finite_vertices_iterator vit = tr.finite_vertices_begin();
       vit != tr.finite_vertices_end(); ++vit) {

    Point p = tr.point(vit);
    int index = get(vertex_pmap, vit);
    assert(index < number_of_meshes);

    if (index >= 0) {
      // how should we consider the case when index == -1 ??
      Vertex_reindexed[vit] = meshes_vertex_reindexation[index];
      meshes_vertex_reindexation[index]++;
      submeshes[index].add_vertex(sme::mesh::Point(CGAL::to_double(p.x()),
                                                   CGAL::to_double(p.y()),
                                                   CGAL::to_double(p.z())));
    }
  }

  typename C3T3::size_type number_of_triangles =
      c3t3.number_of_facets_in_complex();

  if (print_each_facet_twice)
    number_of_triangles += number_of_triangles;

  for (Facet_iterator fit = c3t3.facets_in_complex_begin();
       fit != c3t3.facets_in_complex_end(); ++fit) {

    typename C3T3::Facet f = (*fit);

    // Apply priority among subdomains, to get consistent facet orientation per
    // subdomain-pair interface.
    if (print_each_facet_twice) {
      // NOTE: We mirror a facet when needed to make it consistent with
      // No_patch_facet_pmap_first/second.
      if (f.first->subdomain_index() >
          f.first->neighbor(f.second)->subdomain_index())
        f = tr.mirror_facet(f);
    }

    // Get facet vertices in CCW order.
    Vertex_handle vh1 = f.first->vertex((f.second + 1) % 4);
    Vertex_handle vh2 = f.first->vertex((f.second + 2) % 4);
    Vertex_handle vh3 = f.first->vertex((f.second + 3) % 4);

    // Facet orientation also depends on parity.
    if (f.second % 2 != 0)
      std::swap(vh2, vh3);

    int index = get(facet_pmap, *fit);
    assert(index < number_of_meshes);

    if (index >= 0) {
      auto vertex1 = Vertex_reindexed[vh1];
      auto vertex2 = Vertex_reindexed[vh2];
      auto vertex3 = Vertex_reindexed[vh3];
      submeshes[index].add_face(Index(vertex1), Index(vertex2), Index(vertex3));
    }

    // Add triangle again if needed, with opposite orientation
    if (print_each_facet_twice) {
      if (index >= 0) {
        submeshes[index].add_face(Index(Vertex_reindexed[vh3]),
                                  Index(Vertex_reindexed[vh2]),
                                  Index(Vertex_reindexed[vh1]));
      }
    }
  }
}

template <class C3T3>
void meshes_builder(std::vector<sme::mesh::SMesh> &submeshes,
                    const C3T3 &c3t3) {
  typedef CGAL::Mesh_3::Medit_pmap_generator<C3T3, true, false> Generator;
  typedef typename Generator::Cell_pmap Cell_pmap;
  typedef typename Generator::Facet_pmap Facet_pmap;
  typedef typename Generator::Facet_pmap_twice Facet_pmap_twice;
  typedef typename Generator::Vertex_pmap Vertex_pmap;

  Cell_pmap cell_pmap(c3t3);
  Facet_pmap facet_pmap(c3t3, cell_pmap);
  Facet_pmap_twice facet_pmap_twice(c3t3, cell_pmap);
  Vertex_pmap vertex_pmap(c3t3, cell_pmap, facet_pmap);

  meshes_builder<C3T3>(submeshes, c3t3, vertex_pmap, facet_pmap, cell_pmap,
                       facet_pmap_twice, Generator().print_twice());
}

class Mesh3d {
public:
  //  Mesh3d(double facet_angle_in=30, double facet_size_in=6, double
  //  facet_distance_in=4,
  //         double cell_radius_edge_ratio_in=3, double cell_size_in=8);
  Mesh3d(const sme::model::Model &model, double facet_angle_in = 30,
         double facet_size_in = 6, double facet_distance_in = 4,
         double cell_radius_edge_ratio_in = 3, double cell_size_in = 8);

  void Save(std::string fileName);

  int getNumberOfSubMeshes();

  const sme::mesh::SMesh &GetMesh(int id);

protected:
  C3t3 c3t3;
  double facet_angle_in;
  double facet_size_in;
  double facet_distance_in;
  double cell_radius_edge_ratio_in;
  double cell_size_in;

  std::vector<sme::mesh::SMesh> submeshes;
};

} // namespace sme::mesh

#endif // SPATIALMODELEDITOR_MESH3D_HPP
