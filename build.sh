#!/usr/bin/env bash
#
# Build mdpeek inside a Fedora container and copy the binary to ./build/
#
set -euo pipefail

IMAGE_NAME="mdpeek-builder"
CONTAINER_NAME="mdpeek-build-$$"

echo "==> Building container image..."
podman build -t "${IMAGE_NAME}" .

echo "==> Extracting binary..."
mkdir -p build
podman create --name "${CONTAINER_NAME}" "${IMAGE_NAME}" >/dev/null
podman cp "${CONTAINER_NAME}:/usr/local/bin/mdpeek" ./build/mdpeek
podman rm "${CONTAINER_NAME}" >/dev/null

echo "==> Done: ./build/mdpeek"
ls -lh ./build/mdpeek
