# simple script to download static libraries as used by travis CI builds
# puts them in /opt/libs & removes anything that was there before
rm -rf /opt/libs/*
rm -rf *-static-linux.tgz
for dep in qt5 libsbml dune-copasi
do
   wget https://github.com/lkeegan/$dep-static/releases/latest/download/$dep-static-linux.tgz
   tar xf $dep-static-linux.tgz -C /
done
