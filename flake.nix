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
        buildInputs = with pkgs; [ boost186 spdlog ];
      };
    in
    rec {
      packages = { inherit logovo; };
      devShell = with pkgs; mkShell.override { inherit stdenv; } {
        inputsFrom = [ logovo ];
        nativeBuildInputs = [ gdb clang-tools_18 ];
      };

      packages.default = logovo;
      apps.default.program = "${logovo}/bin/logovo";
      apps.default.type = "app";
    }
  );
}
