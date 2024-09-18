{
  withCcache ? false, # Enable ccache. Requires /nix/var/cache/ccache to exist with correct permissions.

  nixpkgs ? ./pinned.nix,
  overlays ? [
    (import ./overlay.nix {
      inherit withCcache;
      smp = false; # No SMP for chainloader
    })
  ],
  pkgs ? import nixpkgs {
      config = { };
      inherit overlays;
      crossSystem = {
        config = "i686-unknown-linux-musl";
      };
  },
}:
let
  includeos = pkgs.pkgsIncludeOS.includeos;
  stdenv = pkgs.pkgsIncludeOS.stdenv;
in

assert (stdenv.targetPlatform.system != "i686-linux") ->
  throw "Chainloader must be built as 32-bit target";
assert (stdenv.targetPlatform.isLinux == false) ->
  throw "Target platform must be Linux";
assert (stdenv.targetPlatform.isMusl == false) ->
  throw "Target stdenv should be based on Musl";

stdenv.mkDerivation rec {
  pname = "chainloader";
  version = "dev";

  sourceRoot = "./src/chainload/";

  buildInputs = [
    includeos
  ];

  srcs = [
    ./src
    ./api
    ./cmake
    ];

  nativeBuildInputs = [
    pkgs.buildPackages.cmake
    pkgs.buildPackages.nasm
  ];
}
