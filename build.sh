#!/bin/bash

set -e

dir=$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

makepkg DLAGENTS="gdrive::${dir}/gdrive-download.sh %u %o" -s "$@"
