# Nvidia vGPU-Merged Packages for Arch Linux
Use Nvidia vGPU _and_ GeForce functions of consumer-grade NVIDIA GPUs.

This repository contains scripts for building all components of the "merged" driver package into drop-in replacements for the Arch Linux NVIDIA packages. It adds the following additional functions by means of patches/drop-ins:
 - Patched `nvidia-smi` to support recognizing consumer GPUs as vGPU-capable (uses cvgpu.c)
 - Dependency re-ordering to ensure `libvirtd`, if installed, will start _after_ the NVIDIA vGPU services.

## Important
These packages are not guaranteed to work out of the box (or at all), so use it at your own risk. Backups should be taken before attempting to replace stock NVIDIA drivers on any system.

## Installation
1. Download the 460.73.01/02 merged driver package and place it in the root of this repository named `NVIDIA-Linux-x86_64-460.73.01-grid-vgpu-kvm-v5.run` (update the checksum in `PKGBUILD.nvidia-merged` if necessary).
2. Run the `build.sh` script and then install packages from `./out/` as desired.
3. Copy the default `profile_override.toml` into `/etc/vgpu_unlock` and edit.
4. Enable `nvidia-vgpu-mgr.service` and `nvidia-vgpud.service`
4. Reboot.

It is recommended to at least install `nvidia-merged`, `nvidia-merged-settings` and `vgpu_unlock-rs`, which will include the required services/kernel module sources by dependency as well as the Rust edition vGPU-Unlock.

## Credits
This project would not be possible without all those who came before me that wrote the software and figured out how to implement it:
 - [DualCoder - vgpu_unlock](https://github.com/DualCoder/vgpu_unlock)
 - [rupansh - twelve.patch](https://github.com/rupansh/vgpu_unlock_5.12)
 - [HiFiPhile - cvgpu.c](https://gist.github.com/HiFiPhile/b3267ce1e93f15642ce3943db6e60776/)
 - [Krutav Shah - vGPU_Unlock Wiki](https://docs.google.com/document/d/1pzrWJ9h-zANCtyqRgS7Vzla0Y8Ea2-5z2HEi4X75d2Q)
 - [tuh8888 - libvirt_win10_vm](https://github.com/tuh8888/libvirt_win10_vm)
 - [mbilker - vgpu_unlock-rs](https://github.com/mbilker/vgpu_unlock-rs)
