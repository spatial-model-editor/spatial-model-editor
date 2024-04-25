#!/bin/bash

# MacOS Arm64 GUI/CLI build script

set -e -x

# set up certificate to sign macos app, based on:
# - https://localazy.com/blog/how-to-automatically-sign-macos-apps-using-github-actions
# - https://federicoterzi.com/blog/automatic-code-signing-and-notarization-for-macos-apps-using-github-actions/
# - https://docs.github.com/en/actions/deployment/deploying-xcode-applications/installing-an-apple-certificate-on-macos-runners-for-xcode-development
# - https://gist.github.com/gubatron/5512786ff01885c32247ccecd4c3c369
echo -n "$MACOS_CERTIFICATE" | base64 --decode -o certificate.p12
echo "$MACOS_CERTIFICATE" | wc -c
echo -n "$MACOS_CERTIFICATE" | wc -c
cat certificate.p12 | wc -c
security create-keychain -p "$MACOS_KEYCHAIN_PWD" build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p "$MACOS_KEYCHAIN_PWD" build.keychain
security import certificate.p12 -k build.keychain -P "$MACOS_CERTIFICATE_PWD" -T /usr/bin/codesign
security set-key-partition-list -S apple-tool:,apple:,codesign: -s -k "$MACOS_KEYCHAIN_PWD" build.keychain
security list-keychain -d user -s build.keychain
security find-identity -v -p codesigning
