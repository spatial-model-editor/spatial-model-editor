# simple script to download static libraries as used by linux CI builds
# puts them in /opt/smelibs & removes anything that was there before
rm -rf /opt/smelibs
rm -f sme_deps_linux.tgz
wget https://github.com/spatial-model-editor/sme_deps/releases/latest/download/sme_deps_linux.tgz
tar xf sme_deps_linux.tgz -C /
