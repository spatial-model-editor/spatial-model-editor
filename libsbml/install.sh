# script to download & compile libSBML experimental branch including spatial extension
# creates `include` and `lib` directories in current directory
svn co https://svn.code.sf.net/p/sbml/code/branches/libsbml-experimental tmp
cd tmp
mkdir build
cd build
cmake -G Ninja -DENABLE_SPATIAL=ON -DCMAKE_INSTALL_PREFIX=../../ ..
ninja install
cd ../../
rm -rf tmp
