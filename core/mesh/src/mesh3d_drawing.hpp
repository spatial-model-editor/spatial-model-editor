//
// Created by acaramizaru on 6/27/23.
//

#ifndef SPATIALMODELEDITOR_MESH3D_DRAWING_H
#define SPATIALMODELEDITOR_MESH3D_DRAWING_H

#include<CGAL/Qt/Basic_viewer_qt.h>

//a struct that describes 3d segment, Point is the data structure of vertex of segment.
template<typename Point>
struct mySegment3d {
  Point begin;
  Point end;
  mySegment3d() {}
  mySegment3d(Point b,Point e):begin(b),end(e){}
};

#ifdef CGAL_USE_BASIC_VIEWER

#include<CGAL/Qt/init_ogl_context.h>

//viewer for mySegment3d
//Segs3 is an array of mySegment3d which can be traveled by iterator, such as std::veector<mySegment3d>.
template<class Segs3,class point>
class SimpleSegments3ViewerQt :public CGAL::Basic_viewer_qt
{
  typedef Basic_viewer_qt         Base;
  typedef mySegment3d<point>      mySegment3d;

public:
  //construct the viewer
  SimpleSegments3ViewerQt(QWidget* parent,
                          const Segs3& seg3,
                          const char* title = "Basic Segs3 Viewer") : //First draw: vertices, edges, faces, multi-color, no inverse normal
                                                                      Base(parent, title, true, true, false, false, true),
                                                                      s3(seg3)
  {
    compute_elements();
  }
protected:
  const Segs3& s3;
protected:
  void compute_edge(const mySegment3d& seg) {
    add_segment(seg.begin, seg.end, CGAL::IO::blue());
  }
  void compute_vertex(const mySegment3d& seg) {
    add_point(seg.begin, CGAL::IO::red());
    add_point(seg.end,CGAL::IO::red());
  }
  void compute_elements() {
    clear();
    for (auto itor = s3.begin(); itor != s3.end(); ++itor) {
      compute_vertex(*itor);
      compute_edge(*itor);
    }
  }
  virtual void keyPressEvent(QKeyEvent* e) {
    Base::keyPressEvent(e);
  }
};


//draw function
template<typename Segs3,typename Point>
void draw(const Segs3& s3, const char* title = "Segs3 Basic Viewer") {
#if defined(CGAL_TEST_SUITE)
  bool cgal_test_suite = true;
#else
  bool cgal_test_suite = qEnvironmentVariableIsSet("CGAL_TEST_SUITE");
#endif
  if (!cgal_test_suite) {
    CGAL::Qt::init_ogl_context(4, 3);
    int argc = 1;
    const char* argv[2] = { "segs3_viewer","\0" };
    QApplication app(argc, const_cast<char**>(argv));

    SimpleSegments3ViewerQt<Segs3,Point> mainwindow(app.activeWindow(),
                                                     s3, title);
    mainwindow.show();
    app.exec();
  }
}

#endif


#endif // SPATIALMODELEDITOR_MESH3D_DRAWING_H
