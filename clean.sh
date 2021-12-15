#!/bin/bash

set -e

dir=$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)

rm -r -f -v out/ pkg/ src/ vgpu_unlock-rs/
