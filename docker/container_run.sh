#!/bin/bash
# Launch lidar_nav2 image without docker compose.
# Usage (from repo root or docker/):
#   ./docker/container_run.sh <container_name> <image_name:tag>
# Example:
#   ./docker/container_run.sh lidar_nav2 lidar_nav2:humble-cuda
#
# Build the image with the same host user first, e.g.:
#   docker compose build
#   # or:
#   docker build -t lidar_nav2:humble-cuda -f docker/Dockerfile \
#     --build-arg USERNAME=$(whoami) --build-arg USER_UID=$(id -u) \
#     --build-arg USER_GID=$(id -g) .

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# Repo root (parent of docker/)
PROJECT_DIR="$(dirname -- "$SCRIPT_DIR")"

HOST_USER="${USER:-$(whoami)}"
WORKSPACE_IN_CONTAINER="/home/${HOST_USER}/ros2_ws"

echo "Host user:      $HOST_USER"
echo "Host workspace: $PROJECT_DIR"
echo "Container mount: $WORKSPACE_IN_CONTAINER"

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <container_name> <image_name:tag>"
  exit 1
fi

CONTAINER_NAME="$1"
IMAGE_NAME="$2"

# Allow GUI apps (RViz / Gazebo) to use the host display
xhost +local:docker >/dev/null 2>&1 || true

docker run --privileged -it --rm \
  --network host \
  --ipc host \
  --shm-size=4gb \
  --name="$CONTAINER_NAME" \
  -e DISPLAY="${DISPLAY:-:0}" \
  -e NVIDIA_VISIBLE_DEVICES=all \
  -e NVIDIA_DRIVER_CAPABILITIES=compute,utility,graphics \
  -e GZ_VERSION=fortress \
  -e USERNAME="$HOST_USER" \
  -e WORKSPACE="$WORKSPACE_IN_CONTAINER" \
  -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
  -v "$PROJECT_DIR:$WORKSPACE_IN_CONTAINER:rw" \
  "$IMAGE_NAME" \
  bash
