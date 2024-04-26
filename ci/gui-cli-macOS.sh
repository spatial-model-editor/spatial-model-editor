#!/bin/bash

# MacOS GUI/CLI build script

set -e -x

retry () {
    "$@" || (sleep 1 && "$@") || (sleep 3 && "$@") || (sleep 5 && "$@") || echo "$* failed"
}

# do build
mkdir build
cd build
cmake .. \
    -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_PREFIX_PATH="/opt/smelibs;/opt/smelibs/lib/cmake" \
    -DCMAKE_CXX_FLAGS="-DNDEBUG -fpic -fvisibility=hidden" \
    -DSME_LOG_LEVEL=OFF \
    -DFREETYPE_LIBRARY_RELEASE=/opt/smelibs/lib/libQt6BundledFreetype.a \
    -DFREETYPE_INCLUDE_DIR_freetype2=/opt/smelibs/include/QtFreetype \
    -DFREETYPE_INCLUDE_DIR_ft2build=/opt/smelibs/include/QtFreetype \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="11"
ninja --verbose

# run cpp tests
time ./test/tests -as ~[gui] > tests.txt 2>&1 || (tail -n 10000 tests.txt && exit 1)
tail -n 100 tests.txt

# run python tests
cd sme
python -m pip install -r ../../sme/requirements-test.txt
python -m pytest ../../sme/test -v
PYTHONPATH=`pwd` python ../../sme/test/sme_doctest.py -v
cd ..

# run benchmarks (~1 sec per benchmark, ~20secs total)
time ./benchmark/benchmark 1

# strip binaries
du -sh app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
du -sh cli/spatial-cli

strip app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
strip cli/spatial-cli

du -sh app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
du -sh cli/spatial-cli

# check dependencies of binaries
otool -L app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor
otool -L cli/spatial-cli

# create iconset & copy into app
mkdir -p app/spatial-model-editor.app/Contents
mkdir -p app/spatial-model-editor.app/Contents/Resources
iconutil -c icns -o app/spatial-model-editor.app/Contents/Resources/icon.icns ../core/resources/icon.iconset

# set up certificate to sign macos app, based on:
# - https://localazy.com/blog/how-to-automatically-sign-macos-apps-using-github-actions
# - https://federicoterzi.com/blog/automatic-code-signing-and-notarization-for-macos-apps-using-github-actions/
# - https://docs.github.com/en/actions/deployment/deploying-xcode-applications/installing-an-apple-certificate-on-macos-runners-for-xcode-development
# - https://gist.github.com/gubatron/5512786ff01885c32247ccecd4c3c369
echo -n "$MACOS_CERTIFICATE" | base64 --decode -o certificate.p12
security create-keychain -p "$MACOS_KEYCHAIN_PWD" build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p "$MACOS_KEYCHAIN_PWD" build.keychain
security import certificate.p12 -k build.keychain -P "$MACOS_CERTIFICATE_PWD" -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$MACOS_KEYCHAIN_PWD" build.keychain
security list-keychain -d user -s build.keychain
security find-identity -v -p codesigning

# sign app and binary
/usr/bin/codesign --force -s "$MACOS_CERTIFICATE_NAME" --options runtime --entitlements ../entitlements.plist app/spatial-model-editor.app -v
/usr/bin/codesign --force -s "$MACOS_CERTIFICATE_NAME" --options runtime --entitlements ../entitlements.plist cli/spatial-cli -v
/usr/bin/codesign -v app/spatial-model-editor.app

# notarize app and binary
# store credentials to avoid UI prompts for passwords
xcrun notarytool store-credentials "notarytool-profile" --apple-id "$MACOS_NOTARIZATION_APPLE_ID" --team-id "$MACOS_NOTARIZATION_TEAM_ID" --password "$MACOS_NOTARIZATION_PWD"

# zip GUI app
ditto -c -k --keepParent "app/spatial-model-editor.app" "notarization_gui.zip"
# submit to be notarized
xcrun notarytool submit "notarization_gui.zip" --keychain-profile "notarytool-profile" --wait
# attach resulting staple to app for offline validation
xcrun stapler staple "app/spatial-model-editor.app"
# repeat for CLI - but note that although we can notarize a command line binary we can't staple it
ditto -c -k --keepParent "cli/spatial-cli" "notarization_cli.zip"
xcrun notarytool submit "notarization_cli.zip" --keychain-profile "notarytool-profile" --wait

# make dmg of GUI app
retry hdiutil create spatial-model-editor-"${RUNNER_ARCH}" -fs HFS+ -srcfolder app/spatial-model-editor.app
# notarize & staple the dmg
ditto -c -k --keepParent "spatial-model-editor-${RUNNER_ARCH}.dmg" "notarization_gui_dmg.zip"
xcrun notarytool submit "notarization_gui_dmg.zip" --keychain-profile "notarytool-profile" --wait
xcrun stapler staple "spatial-model-editor-${RUNNER_ARCH}.dmg"

# make dmg of CLI
mkdir spatial-cli
cp cli/spatial-cli spatial-cli/.
retry hdiutil create spatial-cli-"${RUNNER_ARCH}" -fs HFS+ -srcfolder spatial-cli
# notarize & staple the dmg
ditto -c -k --keepParent "spatial-cli-${RUNNER_ARCH}.dmg" "notarization_cli_dmg.zip"
xcrun notarytool submit "notarization_cli_dmg.zip" --keychain-profile "notarytool-profile" --wait
xcrun stapler staple "spatial-cli-${RUNNER_ARCH}.dmg"

# display version
./app/spatial-model-editor.app/Contents/MacOS/spatial-model-editor -v

# move binaries to artifacts/binaries
cd ..
mkdir -p artifacts/binaries
mv build/"spatial-model-editor-${RUNNER_ARCH}.dmg" artifacts/binaries/
mv build/"spatial-cli-${RUNNER_ARCH}.dmg" artifacts/binaries/
