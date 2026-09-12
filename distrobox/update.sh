#!/usr/bin/env bash
set -euo pipefail

readonly VALID_DISTROS=(jazzy kilted lyrical rolling)

usage() {
  echo "Usage: $0 <distro>" >&2
  echo "  distro: ${VALID_DISTROS[*]}" >&2
  exit 1
}

ros_distro="${1:-}"
if [[ -z "${ros_distro}" ]]; then
  usage
fi

if [[ ! " ${VALID_DISTROS[*]} " =~ " ${ros_distro} " ]]; then
  echo "Unknown ROS distro: ${ros_distro}" >&2
  usage
fi

readonly container_name="rqt-multiplot-${ros_distro}"
readonly image_name="rqt-multiplot:${ros_distro}"
readonly distrobox_path="$(dirname "$(realpath "$0")")"
readonly current_path="$(pwd)"
readonly start_time="$(date +%s)"

processes="$(pgrep distrobox-enter || true)"
has_running_processes=false
for process in ${processes}; do
  command="$(ps -p "${process}" -o cmd=)"
  if [[ "${command}" =~ distrobox-enter( -n)?\ ${container_name} ]]; then
    has_running_processes=true
    echo "Process still running (${process}): '${command}'"
    echo "Killing process ${process}: '${command}'"
    kill -s 15 "${process}"
  fi
done

if [[ "${has_running_processes}" == true ]]; then
  echo "Stop running processes!" >&2
  exit 1
fi

if docker ps -a --format '{{.Names}}' | grep -qw "${container_name}"; then
  distrobox stop --yes "${container_name}" || true
  echo "Stop container '${container_name}'"
  docker stop "${container_name}" || true
  echo "Delete container '${container_name}'"
  docker rm -f "${container_name}" || true
fi

cd "${distrobox_path}"

echo "Pull base image 'ros:${ros_distro}'"
docker pull "ros:${ros_distro}"

echo "Build image '${image_name}'"
docker buildx bake --allow=fs.read=.. --builder default --load "${ros_distro}"

host_rqt_config="${HOME}/.config/ros.org-${ros_distro}"
mkdir -p "${host_rqt_config}"

echo "Write Distrobox config file '${ros_distro}.ini'"
cat <<EOF > "${ros_distro}.ini"
[${container_name}]
image=${image_name}
nvidia=false
start_now=true
init=false
volume="/run/dbus/system_bus_socket:/run/dbus/system_bus_socket "
volume="\$HOME/.config/ros.org-${ros_distro}:\$HOME/.config/ros.org"
hostname=\$HOSTNAME
additional_flags="--device /dev/dri "
additional_flags="--env ROS_VERSION=2 "
additional_flags="--env ROS_DISTRO=${ros_distro} "
additional_flags="--env QT_QPA_PLATFORM=xcb "
unshare_process=true
EOF

echo "Create container '${container_name}'"
distrobox assemble create --file "${ros_distro}.ini"

end_time="$(date +%s)"
echo "Done. Execution time: $((end_time - start_time)) seconds"

cd "${current_path}"
