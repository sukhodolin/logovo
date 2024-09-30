# Logovo - log tail over HTTP

This is a web server that serves logs from a given directory over HTTP. It's assumed that logs
are lines of text with newest at the end of file.

# How to build

Recommended way is to have [Nix](https://nixos.org/) installed with flakes
enabled ([The Determinate Nix Installer](https://github.com/DeterminateSystems/nix-installer)
enables them by default, for other install options see [NixOS wiki](https://nixos.wiki/wiki/Flakes))
then just run `nix run` in this directory.

If you don't want to bother with Nix, you will need to take care of dependencies by yourself.
The project needs:

- Relatively up-to-date GCC or Clang (I've tested with GCC 13)
- Recent Boost (Nix environment uses Boost 1.76)
- spdlog
- Google Test

# Dev shell

If you have Nix, you also may enter the development shell with all dependencies above ready to be
used. To do that run `nix develop`.

Once you have a shell with dependencies (either by Nix or by any other method including installing
dependencies manually), you can build the project:

- `cmake -H. -Bbuild/debug`
- `cmake --build build/debug`
- `./build/debug/logovo/logovo`

# Generate test data

There is a small tool (not terribly performant as well) available to generate huge files with
"logs". You could use it like this:

- `mkdir -p log_root`
- `nix run .#loggen -- log_root/log.txt --lines-number 500000000` (this will be about 19 gigs of data)
