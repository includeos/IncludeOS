{ withCcache ? false,

  nixpkgs ? ./pinned.nix,
  includeos ? import ./default.nix { inherit withCcache; },
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

  version = "dev";
}
