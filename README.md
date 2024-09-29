# How to build and run

Recommended way is to have [Nix](https://nixos.org/) installed with flakes
enabled ([The Determinate Nix Installer](https://github.com/DeterminateSystems/nix-installer)
enables them by default, for other install options see [NixOS wiki](https://nixos.wiki/wiki/Flakes))
then just run `nix run` in this directory.

If you don't want to bother with Nix, go with the regular C++ routine:

- Make sure you have [boost](https://www.boost.org/) installed (with a dev package containing headers)
- `cmake -H. -Bbuild/debug`
- `cmake --build build/debug`
- `./build/debug/logovo/logovo`

# Requirements

- GCC 14+
