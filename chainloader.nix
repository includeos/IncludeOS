{ nixpkgs ? ./pinned.nix,
  overlays ? [
    (import ./overlay.nix)
  ],
  pkgs ? import nixpkgs {
      config = { };
      inherit overlays;
      crossSystem = {
        config = "i686-unknown-linux-musl";
      };
  },
  llvmPkgs ? pkgs.llvmPackages_18
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

  cmakeFlags = [
    "-DINCLUDEOS_PACKAGE=${includeos}"
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

  buildInputs = [
    pkgs.microsoft_gsl
  ];
}
