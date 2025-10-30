{ nixpkgs ? ./pinned.nix,
  pkgs ? import nixpkgs { config = { }; overlays = [ ]; },
  stdenv ? pkgs.llvmPackages_19.libcxxStdenv,
  withCcache ? false,
}:
let
  ccache = pkgs.callPackage ./nix/ccache.nix { };
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
  libfmt = pkgs.callPackage ./deps/libfmt { stdenv = stdenv; };

  passthru = {
    inherit lest;
    inherit libfmt;
  };

  nativeBuildInputs = [
    pkgs.buildPackages.cmake
    pkgs.buildPackages.valgrind
    pkgs.buildPackages.clang-tools
  ] ++ pkgs.lib.optionals withCcache [ccache.wrapper];

  buildInputs = [
    pkgs.rapidjson
    pkgs.http-parser
    pkgs.openssl
    lest
    uzlib
    libfmt
  ];
}
