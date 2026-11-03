#!/usr/bin/env bash
# Rolling rclcpp removed topic statistics. Released ros_babel_fish 5.26.90 still
# uses it, so the Ubuntu package is gone and the RPM headers do not compile.
# Build c6f4bb23 with that use removed and install it into /opt/ros/rolling.

set -euo pipefail

if [[ ${ROS_DISTRO:-} != rolling ]]; then
  exit 0
fi

if [[ -f /etc/profile.d/ros-os-override.sh ]]; then
  # shellcheck disable=SC1091
  source /etc/profile.d/ros-os-override.sh
fi

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd "${script_dir}/.." && pwd)
src_dir=$(cd "${repo_root}/.." && pwd)
if [[ $(basename "${src_dir}") != src ]]; then
  echo "run this from a workspace that checked the repo out under src/" >&2
  exit 1
fi

sha=c6f4bb23b79973c12a5b7c1bdd9d28f1b7fd861d
clone=${src_dir}/ros_babel_fish
patch_file=${repo_root}/patches/ros_babel_fish-rolling-topic-statistics.patch
prefix=/opt/ros/${ROS_DISTRO}

if [[ ! -d ${clone}/.git ]]; then
  rm -rf "${clone}"
  git init -q -b main "${clone}"
  git -C "${clone}" remote add origin https://github.com/LOEWE-emergenCITY/ros_babel_fish.git
  git -C "${clone}" fetch --depth 1 origin "${sha}"
  git -C "${clone}" checkout --detach FETCH_HEAD
fi

head=$(git -C "${clone}" rev-parse HEAD)
if [[ ${head} != "${sha}" ]]; then
  echo "ros_babel_fish at ${clone} is ${head}, expected ${sha}" >&2
  exit 1
fi

stamp=${clone}/.rolling-topic-statistics-patched
if [[ ! -f ${stamp} ]]; then
  git -C "${clone}" apply "${patch_file}"
  touch "${stamp}"
fi

set +u
# shellcheck disable=SC1091
source "${prefix}/setup.bash"
set -u

rosdep install --from-paths "${clone}/ros_babel_fish" --ignore-src -y \
  --rosdistro "${ROS_DISTRO}" \
  --dependency-types buildtool \
  --dependency-types build \
  --dependency-types build_export

build_dir=$(mktemp -d)
cmake -S "${clone}/ros_babel_fish" -B "${build_dir}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${prefix}" \
  -DBUILD_TESTING=OFF
cmake --build "${build_dir}" --parallel "$(nproc)"

if command -v dnf >/dev/null 2>&1; then
  if ! dnf install -y "ros-${ROS_DISTRO}-ros-babel-fish-devel"; then
    echo "ros-babel-fish rpm is not installable; the source build still provides it" >&2
  fi
fi

cmake --install "${build_dir}"
rm -rf "${build_dir}"

header=${prefix}/include/ros_babel_fish/detail/babel_fish_subscription.hpp
if [[ ! -f ${header} ]] || grep -q topic_statistics "${header}"; then
  echo "patched ros_babel_fish header is not installed at ${header}" >&2
  exit 1
fi

touch "${clone}/COLCON_IGNORE"
