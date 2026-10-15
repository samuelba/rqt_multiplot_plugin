#!/usr/bin/env bash
# Generate an RPM and build it without running %check.
# Jazzy on RHEL 9 uses bloom-generate rosrpm. Other pairs use generate_rosdynrpm.py.

set -euo pipefail

dest=${1:?output directory}
: "${ROS_DISTRO:?ROS_DISTRO is required}"

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
if [[ -f /etc/profile.d/ros-os-override.sh ]]; then
  # shellcheck disable=SC1091
  . /etc/profile.d/ros-os-override.sh
fi
cd "${root}"
rm -rf rpm

# shellcheck disable=SC1091
. /etc/os-release

if [[ ${ID} != fedora && ${VERSION_ID} == 9* && ${ROS_DISTRO} == jazzy ]]; then
  bloom-generate rosrpm \
    --os-name rhel \
    --os-version 9 \
    --ros-distro "${ROS_DISTRO}" \
    "${root}"
else
  if [[ ${ID} == fedora || ${VERSION_ID} != 9* ]]; then
    dnf install -y bloom-rpm-macros
  fi
  python3 "${root}/scripts/generate_rosdynrpm.py" "${ROS_DISTRO}" "${root}"
fi

spec=$(find "${root}/rpm" -name '*.spec' -print -quit)
if [[ -z ${spec} ]]; then
  echo "no spec generated under ${root}/rpm" >&2
  exit 1
fi

eval "$("${root}/scripts/continuous_deb_version.sh" --spec "${spec}")"
echo "Continuous RPM version: ${CONTINUOUS_VERSION}"

name="ros-${ROS_DISTRO}-rqt-multiplot"
spec_name=$(awk '/^Name:/ { print $2; exit }' "${spec}")
if [[ ${spec_name} != "${name}" ]]; then
  echo "spec Name is ${spec_name}, expected ${name}" >&2
  exit 1
fi

topdir=$(mktemp -d)
mkdir -p "${topdir}/SOURCES" "${topdir}/SPECS" "${topdir}/BUILD" "${topdir}/RPMS" "${topdir}/SRPMS"
parent=$(dirname "${root}")
base=$(basename "${root}")
tar -C "${parent}" -czf "${topdir}/SOURCES/${name}-${CONTINUOUS_VERSION}.tar.gz" \
  --exclude='.git' \
  --transform "s,^${base},${name}-${CONTINUOUS_VERSION}," \
  "${base}"

# %generate_buildrequires asks for ros-<distro>(ros_workspace)(devel).
# rosdep installs the runtime package only.
if [[ ${ID} == fedora || ${VERSION_ID} != 9* || ${ROS_DISTRO} != jazzy ]]; then
  dnf install -y "ros-${ROS_DISTRO}-ros-workspace-devel"
fi

rpmbuild -ba \
  --without tests \
  --define "_topdir ${topdir}" \
  "${spec}"

mkdir -p "${dest}"
mapfile -d '' rpms < <(find "${topdir}/RPMS" -type f -name '*.rpm' \
  ! -name '*-debuginfo-*' \
  ! -name '*-debugsource-*' -print0)
if [[ ${#rpms[@]} -eq 0 ]]; then
  echo "rpmbuild produced no packages" >&2
  exit 1
fi
cp -t "${dest}" "${rpms[@]}"
ls -la "${dest}"
