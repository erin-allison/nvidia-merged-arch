#!/bin/bash

dir=$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

PKGDEST="${dir}/out" makepkg -p PKGBUILD.nvidia-merged
