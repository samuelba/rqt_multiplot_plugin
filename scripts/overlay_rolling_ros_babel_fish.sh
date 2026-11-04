#!/usr/bin/env bash
# Rolling rclcpp removed topic statistics. Released ros_babel_fish 5.26.90 still
# uses it, so the Ubuntu package is gone and the RPM headers do not compile.
# Build c6f4bb23 with that use removed and install it into /opt/ros/rolling.
#
# --check-expiry exits 1 when that distro package installs and its subscription
# header no longer uses topic_statistics. CI fails on that so the overlay can
# be removed.

set -euo pipefail

# True when the installed header shows the distro package no longer needs the patch.
distro_header_is_fixed() {
  local header=$1
  [[ -f ${header} ]] && ! grep -q topic_statistics "${header}"
}

check_expiry() {
  local pkg header
  header="/opt/ros/${ROS_DISTRO}/include/ros_babel_fish/detail/babel_fish_subscription.hpp"

  if command -v dnf >/dev/null 2>&1; then
    pkg="ros-${ROS_DISTRO}-ros-babel-fish-devel"
    echo "Checking whether ${pkg} still needs the topic_statistics overlay."
    if ! dnf install -y "${pkg}"; then
      echo "${pkg} is not installable. The rolling overlay is still required."
      exit 0
    fi
  elif command -v apt-get >/dev/null 2>&1; then
    export DEBIAN_FRONTEND="${DEBIAN_FRONTEND:-noninteractive}"
    pkg="ros-${ROS_DISTRO}-ros-babel-fish"
    echo "Checking whether ${pkg} still needs the topic_statistics overlay."
    apt-get update
    if ! apt-get install -y --no-install-recommends "${pkg}"; then
      echo "${pkg} is not installable. The rolling overlay is still required."
      exit 0
    fi
  else
    echo "No dnf or apt-get; cannot check the rolling ros_babel_fish package." >&2
    exit 1
  fi

  if [[ ! -f ${header} ]]; then
    echo "${pkg} is installed but ${header} is absent. The rolling overlay is still required."
    exit 0
  fi

  if distro_header_is_fixed "${header}"; then
    echo "::error title=ros_babel_fish workaround expired::${pkg} no longer uses topic_statistics. Drop the overlay for this OS. Delete scripts/overlay_rolling_ros_babel_fish.sh and patches/ros_babel_fish-rolling-topic-statistics.patch when every expiry job is red."
    exit 1
  fi

  echo "${pkg} still uses topic_statistics. The rolling overlay is still required."
  exit 0
}

if [[ ${BASH_SOURCE[0]} != "$0" ]]; then
  return 0
fi

if [[ $# -gt 1 ]]; then
  echo "usage: $0 [--check-expiry]" >&2
  exit 2
fi

if [[ ${1:-} == --check-expiry ]]; then
  if [[ ${ROS_DISTRO:-} != rolling ]]; then
    echo "ROS_DISTRO must be rolling to check the ros_babel_fish overlay." >&2
    exit 1
  fi
  check_expiry
fi

if [[ $# -gt 0 ]]; then
  echo "usage: $0 [--check-expiry]" >&2
  exit 2
fi

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
