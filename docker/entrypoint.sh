#!/usr/bin/env bash
set -e

source "/opt/ros/${ROS_DISTRO}/setup.bash"

if [ -f "${WORKSPACE}/install/setup.bash" ]; then
  # shellcheck disable=SC1091
  source "${WORKSPACE}/install/setup.bash"
fi

if [ -d "${WORKSPACE}" ]; then
  cd "${WORKSPACE}"
fi

exec "$@"
