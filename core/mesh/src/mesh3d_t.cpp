//
// Created by acaramizaru on 6/26/23.
//

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include <CGAL/Mesh_triangulation_3.h>
#include <CGAL/Mesh_complex_3_in_triangulation_3.h>
#include <CGAL/Mesh_criteria_3.h>

#include <CGAL/Labeled_mesh_domain_3.h>
#include <CGAL/make_mesh_3.h>
#include <CGAL/Image_3.h>

// Domain
typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Labeled_mesh_domain_3<K> Mesh_domain;

#ifdef CGAL_CONCURRENT_MESH_3
typedef CGAL::Parallel_tag Concurrency_tag;
#else
typedef CGAL::Sequential_tag Concurrency_tag;
#endif

// Triangulation
typedef CGAL::Mesh_triangulation_3<Mesh_domain,CGAL::Default,Concurrency_tag>::type Tr;

typedef CGAL::Mesh_complex_3_in_triangulation_3<Tr> C3t3;

// Criteria
typedef CGAL::Mesh_criteria_3<Tr> Mesh_criteria;

// To avoid verbose function and named parameters call
using namespace CGAL::parameters;


// *****
#include "catch_wrapper.hpp"
#include "sme/mesh3d.hpp"
#include "sme/utils.hpp"

#include "sme/model.hpp"
#include "sme/image_stack.hpp"

using namespace sme;

// *****


//#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
//#include <CGAL/Polygon_2.h>
//#include <CGAL/Polygon_with_holes_2.h>
//#include <CGAL/Polygon_set_2.h>
//#include <CGAL/draw_polygon_set_2.h>
//
//typedef CGAL::Exact_predicates_exact_constructions_kernel K_1;
//typedef CGAL::Polygon_2<K_1>                            Polygon_2;
//typedef CGAL::Polygon_with_holes_2<K_1>                 Polygon_with_holes_2;
//typedef CGAL::Polygon_set_2<K_1>                        Polygon_set_2;
//typedef CGAL::Point_2<K_1>                              Point_2;
//
//Polygon_2 rectangle(int l)
//{
//  // Create a rectangle with given side length.
//  Polygon_2 P;
//  P.push_back(Point_2(-l,-l));
//  P.push_back(Point_2(l,-l));
//  P.push_back(Point_2(l,l));
//  P.push_back(Point_2(-l,l));
//
//  return P;
//}

// *****

#define CGAL_USE_BASIC_VIEWER

#include<vector>
#include<CGAL/Simple_cartesian.h>
#include"mesh3d_drawing.hpp"
typedef CGAL::Simple_cartesian<double> kernel;
typedef kernel::Point_3 Point_3;

// *****

TEST_CASE("Mesh3d", "[core/mesh/mesh3d][core/mesh][core][mesh]") {

  SECTION("first test") {
    REQUIRE(TRUE);
  }

  SECTION("mesh3d_drawing") {

    std::vector<mySegment3d<Point_3>> segs;
    Point_3 p0(0, 0, 0), p1(1, 2, 3), p2(5, 3, 1), p3(3, 1, 10);
    mySegment3d<Point_3> s0(p0, p1), s1(p0, p2), s2(p0, p3);
    segs.emplace_back(s0);
    segs.emplace_back(s1);
    segs.emplace_back(s2);
    draw<std::vector<mySegment3d<Point_3>>,Point_3>(segs);

  }

//  SECTION("draw polygont set") {
//    // Create a large rectangle A, with a hole and a smaller rectangle
//    // B inside A's hole.
//    Polygon_with_holes_2 A(rectangle(3));
//    Polygon_2 H(rectangle(2));
//    H.reverse_orientation();
//    A.add_hole(H);
//    Polygon_2 B(rectangle(1));
//
//    // Add them to a polygon set and draw it.
//    Polygon_set_2 S;
//    S.insert(A);
//    S.insert(B);
//
//    CGAL::draw(S);
//  }

  SECTION("CGAL mesh3d from image, with sme input") {


    auto m = model::Model();
    m.importFile("/home/acaramizaru/Dropbox/Work-shared/Heidelberg-IWR/spatial-model-editor-resources/cell-3d-model.sme");
    REQUIRE(m.getIsValid());


//    auto img_3 = CGAL::Image_3();
//    img_3.read("/home/acaramizaru/Dropbox/Work-shared/Heidelberg-IWR/spatial-model-editor-resources/cell-3d-model.sme");
//    REQUIRE(img_3.is_valid());
    auto images = m.getGeometry().getImages();


    const unsigned char number_of_spheres = 50;
    const int max_radius_of_spheres = 10;
    const int radius_of_big_sphere = 80;
    _image* image_in = _createImage(
        images.volume().width(),
        images.volume().height(),
        images.volume().depth(),
        1, 1.f, 1.f, 1.f, 1,
        WK_FIXED, SGN_UNSIGNED
        );
    unsigned char* ptr = static_cast<unsigned char*>(image_in->data);



    for(auto it = images.begin(); it != images.end(); it++ ) {
      auto image = *it;
      std::memcpy(
          ptr + (it - images.begin()) *
                    images.volume().width() * images.volume().height(),
          image.bits(),
          images.volume().width() * images.volume().height()
          );
    }

    auto image = CGAL::Image_3(image_in);
    REQUIRE(image.is_valid());

    /// [Domain creation]
    Mesh_domain domain = Mesh_domain::create_labeled_image_mesh_domain(image);
    /// [Domain creation]

    // Mesh criteria
    Mesh_criteria criteria(facet_angle=30, facet_size=6, facet_distance=4,
                           cell_radius_edge_ratio=3, cell_size=8);

    /// [Meshing]
    C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria);
    /// [Meshing]

    // Output
    std::ofstream medit_file("out.mesh");
    c3t3.output_to_medit(medit_file);

  }

  /*
  SECTION("CGAL mash3D from image, original") {

    /// [Loads image]
    const std::string fname = "/home/acaramizaru/git/CGAL-5.4.1-examples/CGAL-5.4.1/examples/../data/images/liver.inr.gz";
    CGAL::Image_3 image;
    if(!image.read(fname)){
      std::cerr << "Error: Cannot read file " <<  fname << std::endl;
      REQUIRE(FALSE);
    }
    /// [Loads image]

    /// [Domain creation]
    Mesh_domain domain = Mesh_domain::create_labeled_image_mesh_domain(image);
    /// [Domain creation]

    // Mesh criteria
    Mesh_criteria criteria(facet_angle=30, facet_size=6, facet_distance=4,
                           cell_radius_edge_ratio=3, cell_size=8);

    /// [Meshing]
    C3t3 c3t3 = CGAL::make_mesh_3<C3t3>(domain, criteria);
    /// [Meshing]

    // Output
    std::ofstream medit_file("out.mesh");
    c3t3.output_to_medit(medit_file);

  }
  */


}



