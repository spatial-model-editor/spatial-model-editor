rm -rf lib
rm -rf include
rm -rf *.tgz *.tgz.*
for dep in qt5 libsbml dune-copasi
do
   wget https://github.com/lkeegan/$dep-static/releases/latest/download/$dep-static-linux.tgz
   tar xf $dep-static-linux.tgz
done
mv opt/libs/lib .
mv opt/libs/include .
rm -rf opt
cp -r usr/local/qt5-static/lib/* lib/.
mv usr/local/qt5-static/bin .
mv usr/local/qt5-static/mkspecs .
mv usr/local/qt5-static/plugins .
cp -r usr/local/qt5-static/include/* include/.
rm -rf usr
cp -r dune/lib/* lib/.
cp -r dune/include/* include/.
