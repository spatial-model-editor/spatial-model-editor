# script to extract win64 static library from sourceforge
wget https://sourceforge.net/projects/sbml/files/libsbml/5.18.0/experimental/binaries/Windows/libSBML-5.18.0-win64.zip
unzip libSBML-5.18.0-win64.zip
mv libSBML-5.18.0-win64/lib/libsbml-static.lib .
rm -rf libSBML-5.18.0-win64

# script to download & compile libSBML experimental branch for linux, including spatial extension
# creates `sbml` dir containing header files and `libsbml-static.a` library in the current directory
svn co https://svn.code.sf.net/p/sbml/code/branches/libsbml-experimental tmp
cd tmp
mkdir build
cd build
cmake -G Ninja -DENABLE_SPATIAL=ON -DCMAKE_INSTALL_PREFIX=../install ..
ninja install
cd ../../
mv tmp/install/lib/libsbml-static.a .
mv tmp/install/include/sbml sbml
rm -rf tmp
