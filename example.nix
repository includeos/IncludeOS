{ nixpkgs ? ./pinned.nix,
  includeos ? import ./default.nix { },
  pkgs ? (import nixpkgs { }).pkgsStatic,
  llvmPkgs ? pkgs.llvmPackages_18
}:

includeos.stdenv.mkDerivation rec {
  pname = "includeos_example";
  src = pkgs.lib.cleanSource ./example;
  doCheck = false;
  dontStrip = true;

  nativeBuildInputs = [
    pkgs.buildPackages.nasm
    pkgs.buildPackages.cmake
  ];

  buildInputs = [
    pkgs.microsoft_gsl
    includeos
  ];

  # TODO:
  # We currently need to explicitly pass in because we link with a linker script
  # and need to control linking order.
  # This can be moved to os.cmake eventually, once we figure out how to expose
  # them to cmake from nix without having to make cmake depend on nix.
  # * Maybe we should make symlinks from the includeos package to them.

  cmakeFlags = [
    "-DINCLUDEOS_PACKAGE=${includeos}"
    "-DARCH=x86_64"
    "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"
  ];

  version = "dev";
}
