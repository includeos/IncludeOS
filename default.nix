{ nixpkgs ? ./pinned.nix,
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
