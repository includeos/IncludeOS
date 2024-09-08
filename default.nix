{ withCcache ? false, # Enable ccache. Requires correct permissions, see overlay.nix.
  smp ? false, # Enable multcore support (SMP)
  nixpkgs ? ./pinned.nix,
  overlays ? [
    (import ./overlay.nix { inherit withCcache; inherit smp; } )
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
