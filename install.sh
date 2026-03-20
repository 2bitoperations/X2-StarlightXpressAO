#!/usr/bin/env bash
# install.sh — Build and install the Starlight Xpress AO plugin for TheSkyX
#
# Usage:
#   ./install.sh           # build and install
#   ./install.sh --uninstall  # remove installed files
#
# No sudo is required if you own the TheSkyX app bundle (typical single-user install).
# If you get "Permission denied", run: sudo ./install.sh

set -euo pipefail

# ---------------------------------------------------------------------------
# Platform detection
# ---------------------------------------------------------------------------
UNAME_S="$(uname -s)"
case "${UNAME_S}" in
    Darwin)
        TSX_APP="/Applications/TheSkyX Professional Edition.app"
        PLUGIN_DIR="${TSX_APP}/Contents/PlugIns/AdaptiveOpticsPlugIns"
        LIST_DIR="${TSX_APP}/Contents/Resources/Common/Miscellaneous Files"
        LIB_NAME="libSXAO.dylib"
        ;;
    Linux)
        # Adjust TSX_HOME if TheSkyX is installed elsewhere on this machine.
        TSX_HOME="${HOME}/TheSkyX"
        PLUGIN_DIR="${TSX_HOME}/Resources/Common/PlugIns64/AdaptiveOpticsPlugIns"
        LIST_DIR="${TSX_HOME}/Resources/Common/Miscellaneous Files"
        LIB_NAME="libSXAO.so"
        ;;
    *)
        echo "ERROR: Unsupported platform: ${UNAME_S}" >&2
        exit 1
        ;;
esac

LIST_FILE="adaptiveopticslist StarlightXpress.txt"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---------------------------------------------------------------------------
# Uninstall
# ---------------------------------------------------------------------------
if [[ "${1:-}" == "--uninstall" ]]; then
    echo "Uninstalling SX AO plugin..."
    rm -f "${PLUGIN_DIR}/${LIB_NAME}"  && echo "  Removed ${PLUGIN_DIR}/${LIB_NAME}"
    rm -f "${PLUGIN_DIR}/SXAO.ui"     && echo "  Removed ${PLUGIN_DIR}/SXAO.ui"
    rm -f "${LIST_DIR}/${LIST_FILE}"  && echo "  Removed ${LIST_DIR}/${LIST_FILE}"
    echo "Done."
    exit 0
fi

# ---------------------------------------------------------------------------
# Pre-flight checks
# ---------------------------------------------------------------------------
if [[ ! -d "${PLUGIN_DIR}" ]]; then
    echo "ERROR: Plugin directory not found:" >&2
    echo "  ${PLUGIN_DIR}" >&2
    echo "Is TheSkyX installed?" >&2
    exit 1
fi

if [[ ! -d "${LIST_DIR}" ]]; then
    echo "ERROR: Miscellaneous Files directory not found:" >&2
    echo "  ${LIST_DIR}" >&2
    exit 1
fi

if [[ ! -f "${SCRIPT_DIR}/${LIST_FILE}" ]]; then
    echo "ERROR: Hardware list file not found:" >&2
    echo "  ${SCRIPT_DIR}/${LIST_FILE}" >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
echo "Building ${LIB_NAME}..."
cd "${SCRIPT_DIR}"
make clean
make

if [[ ! -f "${LIB_NAME}" ]]; then
    echo "ERROR: Build failed — ${LIB_NAME} not found." >&2
    exit 1
fi

# ---------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------
echo "Installing plugin..."

cp "${LIB_NAME}" "${PLUGIN_DIR}/${LIB_NAME}"
echo "  -> ${PLUGIN_DIR}/${LIB_NAME}"

cp "SXAO.ui" "${PLUGIN_DIR}/SXAO.ui"
echo "  -> ${PLUGIN_DIR}/SXAO.ui"

cp "${LIST_FILE}" "${LIST_DIR}/${LIST_FILE}"
echo "  -> ${LIST_DIR}/${LIST_FILE}"

echo ""
echo "Installation complete."
echo "Restart TheSkyX and select 'SX AO' from the Adaptive Optics device list."
