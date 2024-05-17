{ nixpkgs ? ./pinned.nix,
  pkgs ? import nixpkgs { config = { }; overlays = [ ]; },
}:

pkgs.stdenv.mkDerivation rec {
  pname = "vmbuild";
  version = "dev";

  sourceRoot = pname;

  srcs = [
    ./vmbuild
    ./src
    ./api
    ];

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.nasm
  ];

  buildInputs = [
    pkgs.microsoft_gsl
  ];

}
