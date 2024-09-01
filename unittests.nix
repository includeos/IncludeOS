{ nixpkgs ? ./pinned.nix,
  pkgs ? import nixpkgs { config = { }; overlays = [ ]; }
}:
let
  stdenv = pkgs.llvmPackages_18.libcxxStdenv;
in
stdenv.mkDerivation rec {
  pname = "unittests";
  version = "dev";
  enableParallelBuilding = true;

  sourceRoot = "test";

  srcs = [
    ./test
    ./src
    ./api
    ./lib
    ];

  hardeningDisable = [ "all" ];
  cmakeBuildType = "Debug";

  lest = pkgs.callPackage ./deps/lest {};
  uzlib = pkgs.callPackage ./deps/uzlib {};

  passthru = {
    inherit lest;
  };

  nativeBuildInputs = [
    pkgs.buildPackages.cmake
    pkgs.buildPackages.valgrind
    pkgs.buildPackages.clang-tools
  ];

  buildInputs = [
    pkgs.rapidjson
    pkgs.http-parser
    pkgs.openssl
    lest
    uzlib
  ];
}
