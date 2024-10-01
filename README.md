# Logovo - log tail over HTTP

This is a web server that serves logs from a given directory over HTTP. It's assumed that logs
are lines of text with newest at the end of file.

# Building & running

Recommended way is to have [Nix](https://nixos.org/) installed with flakes
enabled ([The Determinate Nix Installer](https://github.com/DeterminateSystems/nix-installer)
enables them by default, for other install options see [NixOS wiki](https://nixos.wiki/wiki/Flakes)).

If you have `nix` then you can just `nix run . -- <log dir>` in this directory.

There is also a handy tool to generate dummy log data for testing, so you could run the following
for a quick test:

```
mkdir -p log_root
nix run .#loggen -- log_root/log.txt --lines-number 500000000
nix run . -- log_root
```

After that you can give it a try by running curl:

```
curl --verbose 'localhost:8080/log.txt?n=1000000&grep=13'
```

# Dev shell

If you have Nix, you also may enter the development shell with all dependencies above ready to be
used. To do that run `nix develop`.

# Building without Nix

If you don't want to bother with Nix, you will need to take care of dependencies by yourself.
The project needs:

- Relatively up-to-date GCC or Clang (I've tested with GCC 13)
- Recent Boost (Nix environment uses Boost 1.76)
- spdlog
- Google Test

Once you have a shell with dependencies (either by Nix or by any other method including installing
dependencies manually), you can build the project:

- `cmake -H. -Bbuild/debug`
- `cmake --build build/debug`
- `./build/debug/logovo`

# Command-line flags

`logovo` server supports the following command line flags (you can always run `logovo --help` for
a quick reminder):

- `--log-root <path>` (can be also supplied as an unnamed argument) - path to the root
  directory, defaults to `.`.
- `--listen-at <network address>` - network address to listen at, realistic values are
  `127.0.0.1` or `0.0.0.0`, defaults to `127.0.0.1`.
- `--port <port>` - network port to listen at, defaults to `8080`.
- `--trace` - flag that enables trace-level logging.

# REST API

The server only supports GET requests. Request path is used as the filesystem path to the log file
(relative to the root dir the server was started with). Two request parameters are supported (both optional):

- `n` specifies the number of lines, should be between 0 and 1000000
- `grep` is a filter string for results. If present, only the lines that have the substring with a
  given value will be produced

# Line length caveat

The maximum length of the line in the log file is limited. The limit currently is 64 kilobytes (can
be changed in `liblogovo/tail.h`). If the log file has a line longer than that then the server will
reply with HTTP 200 OK, but will terminate the data transmission once a long line is encountered (so
the client won't be able to tell if there really was an error, or if the server ran out of lines).
Error log message will be logged at the server side, though.

The reason for the limit is to avoid turning the server into a memory bomb. If a line size is unbounded then there's no way to limit the memory size used while reading the log line to detect
where it begins and ends.
