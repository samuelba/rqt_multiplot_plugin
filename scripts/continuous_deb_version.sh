#!/usr/bin/env bash
# Debian upstream version for continuous builds. The tilde sorts below a bloom
# package of the same package.xml version.

set -euo pipefail

changelog=
while [[ $# -gt 0 ]]; do
  case "$1" in
    --changelog)
      changelog=${2:?--changelog requires a path}
      shift 2
      ;;
    *)
      echo "unknown argument: $1" >&2
      exit 1
      ;;
  esac
done

root=$(git rev-parse --show-toplevel)
package_xml="${root}/package.xml"
base=$(sed -n 's/.*<version>\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)<\/version>.*/\1/p' "${package_xml}" | head -n 1)
if [[ -z "${base}" ]]; then
  echo "Could not read <version> from ${package_xml}" >&2
  exit 1
fi

stamp=$(TZ=UTC git -C "${root}" show -s --format=%cd --date=format-local:%Y%m%d%H%M%S HEAD)
# Fixed width. git --short grows when the local history makes 7 characters ambiguous.
sha=$(git -C "${root}" rev-parse HEAD)
sha=${sha:0:7}
continuous_version="${base}~continuous.${stamp}.${sha}"

if [[ -n "${changelog}" ]]; then
  if [[ ! -f "${changelog}" ]]; then
    echo "changelog not found: ${changelog}" >&2
    exit 1
  fi
  tmp=$(mktemp)
  awk -v base="${base}" -v continuous="${continuous_version}" '
    NR == 1 {
      pattern = "(" base "-"
      replacement = "(" continuous "-"
      pos = index($0, pattern)
      if (pos == 0) {
        print "changelog first line does not contain " pattern > "/dev/stderr"
        exit 1
      }
      $0 = substr($0, 1, pos - 1) replacement substr($0, pos + length(pattern))
    }
    { print }
  ' "${changelog}" > "${tmp}"
  mv "${tmp}" "${changelog}"
fi

printf 'BASE=%q\n' "${base}"
printf 'STAMP=%q\n' "${stamp}"
printf 'SHA=%q\n' "${sha}"
printf 'CONTINUOUS_VERSION=%q\n' "${continuous_version}"
