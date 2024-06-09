{ nixpkgs ? ./pinned.nix,
  overlays ? [ (import ./overlay.nix) ],
  pkgs ? import nixpkgs {
    config = {};
    inherit overlays;
  },
  llvmPkgs ? pkgs.llvmPackages
}:
let
  includeos = pkgs.pkgsIncludeOS.includeos;
  stdenv = pkgs.pkgsIncludeOS.stdenv;
in
pkgs.mkShell rec {

  packages = [
    pkgs.buildPackages.cmake
    pkgs.buildPackages.nasm
  ];

  buildInputs = [
    pkgs.microsoft_gsl
  ];

  shellHook = ''
    export CXX=clang++
    export CC=clang
    export bootloader=$INCLUDEOS_PACKAGE/boot/bootloader

    TMPDIR=$(mktemp -d)
    example=$(realpath ../example)
    pushd TMPDIR
    cmake $example -DARCH=x86_64 -DINCLUDEOS_PACKAGE=${includeos}
    # This fails for some reason, due to missing libc includes, but works inside the shell;
    # $ nix-shell --run "make -j12"
    # make -j12
  '';
}
