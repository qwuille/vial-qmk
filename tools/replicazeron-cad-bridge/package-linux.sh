#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
    echo "Usage: $0 EXECUTABLE [OUTPUT_DIRECTORY]" >&2
    exit 2
fi

bridge_root="$(cd "$(dirname "$0")" && pwd)"
executable="$(realpath "$1")"
output_directory="${2:-release}"
mkdir -p "$output_directory"
output_directory="$(realpath "$output_directory")"
staging_directory="$(mktemp -d)"
trap 'rm -rf -- "$staging_directory"' EXIT

package_directory="$staging_directory/ReplicazeronFreeCAD"
cp -R "$bridge_root/freecad/ReplicazeronFreeCAD" "$package_directory"
find "$package_directory" -type d -name __pycache__ -prune -exec rm -rf -- {} +
install -m 0755 "$executable" "$package_directory/ReplicazeronCadBridge"
cp "$bridge_root/README.md" "$package_directory/README.md"

tar -C "$staging_directory" -czf "$output_directory/ReplicazeronFreeCAD-linux-x86_64.tar.gz" ReplicazeronFreeCAD
