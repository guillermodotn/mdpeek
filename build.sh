#!/usr/bin/env bash
#
# Build mdpeek inside a Fedora container and copy the binary to ./build/
#
# Usage:
#   ./build.sh          # GTK4 backend (default)
#   ./build.sh --qt     # Qt6 backend
#
set -euo pipefail

BACKEND="gtk"
if [[ "${1:-}" == "--qt" ]]; then
    BACKEND="qt"
fi

if [[ "${BACKEND}" == "qt" ]]; then
    IMAGE_NAME="mdpeek-builder-qt"
    DOCKERFILE="Dockerfile.qt"
else
    IMAGE_NAME="mdpeek-builder"
    DOCKERFILE="Dockerfile"
fi

CONTAINER_NAME="mdpeek-build-$$"

echo "==> Building ${BACKEND} container image..."
podman build -f "${DOCKERFILE}" -t "${IMAGE_NAME}" .

echo "==> Extracting binary..."
mkdir -p build
podman create --name "${CONTAINER_NAME}" "${IMAGE_NAME}" >/dev/null
podman cp "${CONTAINER_NAME}:/src/build/mdpeek" ./build/mdpeek
podman rm "${CONTAINER_NAME}" >/dev/null

echo "==> Done: ./build/mdpeek"
ls -lh ./build/mdpeek
