#!/bin/bash

set -e

dir=$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

PKGDEST="${dir}/out" makepkg -p PKGBUILD.nvidia-merged -s
PKGDEST="${dir}/out" makepkg -p PKGBUILD.vgpu_unlock-rs -s
