# Lidar_nav2_ws — ROS 2 Humble + CUDA + Gazebo Fortress (Ubuntu 22.04)
# Dependency image only: no workspace COPY / colcon / rosdep-from-src.
# Mount sources at runtime and build packages manually.
#
# Requires NVIDIA Container Toolkit on the host.
#
# Build (uses host username / uid / gid via build args):
#   docker compose build
#   # or:
#   docker build -t lidar_nav2:humble-cuda -f docker/Dockerfile \
#     --build-arg USERNAME=$(whoami) --build-arg USER_UID=$(id -u) \
#     --build-arg USER_GID=$(id -g) .
#
# Run (GPU + GUI):
#   xhost +local:docker
#   docker compose run --rm lidar_nav2 bash
#   nvidia-smi
#   ign gazebo --version   # Fortress

ARG CUDA_IMAGE=nvidia/cuda:12.6.3-devel-ubuntu22.04
ARG ROS_DISTRO=humble
FROM ${CUDA_IMAGE}

ARG ROS_DISTRO=humble
# Host computer user — pass at build time:
#   --build-arg USERNAME=$(whoami) --build-arg USER_UID=$(id -u) --build-arg USER_GID=$(id -g)
ARG USERNAME=ros
ARG USER_UID=1000
ARG USER_GID=100

ENV DEBIAN_FRONTEND=noninteractive \
    ROS_DISTRO=${ROS_DISTRO} \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    USERNAME=${USERNAME} \
    NVIDIA_VISIBLE_DEVICES=all \
    NVIDIA_DRIVER_CAPABILITIES=compute,utility,graphics \
    GZ_VERSION=fortress

SHELL ["/bin/bash", "-c"]

# -----------------------------------------------------------------------------
# System + ROS 2 Humble (CUDA image has no ROS preinstalled)
# -----------------------------------------------------------------------------
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl gnupg lsb-release locales software-properties-common sudo \
    && locale-gen en_US.UTF-8 \
    && curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key \
         -o /usr/share/keyrings/ros-archive-keyring.gpg \
    && echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
         http://packages.ros.org/ros2/ubuntu $(. /etc/os-release && echo "$UBUNTU_CODENAME") main" \
         > /etc/apt/sources.list.d/ros2.list \
    && apt-get update && apt-get install -y --no-install-recommends \
         ros-${ROS_DISTRO}-desktop \
         ros-dev-tools \
    && rm -rf /var/lib/apt/lists/*

# Non-root user (avoids "rosdep update as root" warning)
RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

# Build tools + libs for later manual builds (3d_bbs, LIO, registration, …)
RUN apt-get update && apt-get install -y --no-install-recommends \
    apt apt-utils \
    net-tools \
    vim \
    git wget curl \
    build-essential cmake ninja-build pkg-config \
    python3-pip python3-dev python3-tk python3-numpy python3-matplotlib \
    python3-colcon-common-extensions python3-rosdep python3-vcstool \
    libeigen3-dev libpcl-dev libopencv-dev \
    libboost-all-dev libtbb-dev libomp-dev \
    libgoogle-glog-dev libunwind-dev libflann-dev zlib1g-dev \
    libyaml-cpp-dev \
    qtbase5-dev qtbase5-private-dev libapr1-dev \
    && rm -rf /var/lib/apt/lists/*

# ROS packages + Gazebo Fortress via ros_gz (Humble's default Gazebo pairing)
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-${ROS_DISTRO}-ament-cmake-ros \
    ros-${ROS_DISTRO}-rosidl-default-generators \
    ros-${ROS_DISTRO}-rosidl-default-runtime \
    ros-${ROS_DISTRO}-navigation2 \
    ros-${ROS_DISTRO}-nav2-bringup \
    ros-${ROS_DISTRO}-nav2-msgs \
    ros-${ROS_DISTRO}-nav2-costmap-2d \
    ros-${ROS_DISTRO}-slam-toolbox \
    ros-${ROS_DISTRO}-pointcloud-to-laserscan \
    ros-${ROS_DISTRO}-tf2-tools \
    ros-${ROS_DISTRO}-tf-transformations \
    ros-${ROS_DISTRO}-pcl-ros \
    ros-${ROS_DISTRO}-pcl-conversions \
    ros-${ROS_DISTRO}-cv-bridge \
    ros-${ROS_DISTRO}-image-transport \
    ros-${ROS_DISTRO}-image-geometry \
    ros-${ROS_DISTRO}-laser-geometry \
    ros-${ROS_DISTRO}-message-filters \
    ros-${ROS_DISTRO}-diagnostic-updater \
    ros-${ROS_DISTRO}-rosbag2 \
    ros-${ROS_DISTRO}-xacro \
    ros-${ROS_DISTRO}-robot-state-publisher \
    ros-${ROS_DISTRO}-joint-state-publisher \
    ros-${ROS_DISTRO}-joint-state-publisher-gui \
    ros-${ROS_DISTRO}-ros-gz \
    ros-${ROS_DISTRO}-rtabmap \
    ros-${ROS_DISTRO}-rtabmap-ros \
    && rm -rf /var/lib/apt/lists/*

# GTSAM (global_relocalization_kiss_matcher)
RUN add-apt-repository -y ppa:borglab/gtsam-release-4.1 \
    && apt-get update \
    && apt-get install -y --no-install-recommends libgtsam-dev libgtsam-unstable-dev \
    && ldconfig \
    && rm -rf /var/lib/apt/lists/*

# rosdep init writes to /etc (root); update as non-root
RUN if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then \
      rosdep init; \
    fi

USER ${USERNAME}
RUN rosdep update --rosdistro ${ROS_DISTRO}

USER root
# Empty workspace dir for bind-mounting host sources at runtime
RUN mkdir -p "/home/${USERNAME}/ros2_ws" \
    && chown "${USERNAME}:${USERNAME}" "/home/${USERNAME}/ros2_ws"

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

USER ${USERNAME}

ENV WORKSPACE=/home/${USERNAME}/ros2_ws

WORKDIR ${WORKSPACE}

ENTRYPOINT ["/entrypoint.sh"]
CMD ["bash"]
