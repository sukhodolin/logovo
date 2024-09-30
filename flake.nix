{
  description = "Log server";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/master";
    flake-utils.url = "github:numtide/flake-utils";
    systems.url = "github:nix-systems/default";
  };

  outputs = { self, nixpkgs, flake-utils, systems, ... }: flake-utils.lib.eachSystem (import systems) (system:
    let
      pkgs = import nixpkgs { inherit system; };
      stdenv = pkgs.stdenv;
      logovo = stdenv.mkDerivation {
        name = "logovo";
        src = ./.;
        nativeBuildInputs = with pkgs; [ cmake ];
        buildInputs = with pkgs; [ boost186 spdlog gtest ];
      };
    in
    rec {
      packages = { inherit logovo; };
      devShell = with pkgs; mkShell.override { inherit stdenv; } {
        inputsFrom = [ logovo ];
        nativeBuildInputs = [ gdb llvmPackages_19.clang-tools valgrind ];
      };

      packages.default = logovo;
      apps.default.program = "${logovo}/bin/logovo";
      apps.default.type = "app";
      apps.loggen.program = "${logovo}/bin/loggen";
      apps.loggen.type = "app";
    }
  );
}
