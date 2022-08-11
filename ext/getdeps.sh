# simple script to download static libraries as used by linux CI builds
# puts them in /opt/smelibs & removes anything that was there before
URL=${1:-"https://github.com/spatial-model-editor/sme_deps/releases/latest/download/sme_deps_linux.tgz"}
rm -rf /opt/smelibs
rm -f sme_deps_linux.tgz
wget $URL
tar xf sme_deps_linux.tgz -C /
