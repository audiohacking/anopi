#!/usr/bin/env bash
set -euo pipefail

# Build a simple macOS .pkg that installs ANOPI AU + VST3 (+ Standalone.app).
# Usage: ./scripts/build-installer-pkg.sh --version v0.1.0

VERSION="${2:-0.1.0}"
if [[ "${1:-}" == "--version" ]]; then
  VERSION="${2:-0.1.0}"
fi
VERSION="${VERSION#v}"

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ARTEFACTS="${ROOT}/build/ANOPI_artefacts/Release"
STAGE="${ROOT}/release-artefacts/pkgroot"
OUT_DIR="${ROOT}/release-artefacts"
PKG="${OUT_DIR}/ANOPI-macOS-Installer.pkg"

if [[ ! -d "${ARTEFACTS}" ]]; then
  echo "Missing ${ARTEFACTS}. Build Release first." >&2
  exit 1
fi

rm -rf "${STAGE}"
mkdir -p \
  "${STAGE}/Library/Audio/Plug-Ins/Components" \
  "${STAGE}/Library/Audio/Plug-Ins/VST3" \
  "${STAGE}/Applications"

if [[ -d "${ARTEFACTS}/AU/ANOPI.component" ]]; then
  cp -R "${ARTEFACTS}/AU/ANOPI.component" "${STAGE}/Library/Audio/Plug-Ins/Components/"
fi
if [[ -d "${ARTEFACTS}/VST3/ANOPI.vst3" ]]; then
  cp -R "${ARTEFACTS}/VST3/ANOPI.vst3" "${STAGE}/Library/Audio/Plug-Ins/VST3/"
fi
if [[ -d "${ARTEFACTS}/Standalone/ANOPI.app" ]]; then
  cp -R "${ARTEFACTS}/Standalone/ANOPI.app" "${STAGE}/Applications/"
fi

mkdir -p "${OUT_DIR}"
pkgbuild \
  --root "${STAGE}" \
  --identifier com.anopi.chordbuilder \
  --version "${VERSION}" \
  --install-location / \
  "${PKG}"

echo "Wrote ${PKG}"
