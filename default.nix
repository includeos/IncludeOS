# Nix expression to build IncludeOS.
# See https://nixos.org/nix for info about Nix.
#
# Usage:
#
# $ nix-build ./path/to/this/file.nix
#
# Authors: Bjørn Forsman <bjorn.forsman@gmail.com>

{ nixpkgs ? ./pinned.nix, # Builds cleanly May 9. 2024
  overlays ? [
    (import ./overlay.nix)
  ],
  pkgs ? import nixpkgs { config = {}; inherit overlays; }
}:

let
  inherit (pkgs) pkgsIncludeOS;
in
  assert (pkgsIncludeOS.stdenv.buildPlatform.isLinux == false) ->
    throw "Currently only Linux builds are supported";
  assert (pkgsIncludeOS.stdenv.hostPlatform.isMusl == false) ->
    throw "Stdenv should be based on Musl";

  pkgsIncludeOS.includeos
