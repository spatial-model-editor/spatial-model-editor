# simple script to download static libraries as used by travis CI builds
# puts them in /opt/smelibs & removes anything that was there before
rm -rf /opt/smelibs
for dep in qt5 common dune
do
   rm -f sme_deps_${dep}_linux.tgz
   wget https://github.com/spatial-model-editor/sme_deps_${dep}/releases/latest/download/sme_deps_${dep}_linux.tgz
   tar xf sme_deps_${dep}_linux.tgz -C /
done
