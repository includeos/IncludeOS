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

  lest = stdenv.mkDerivation rec {
    pname = "lest";
    version = "1.35.2";

    meta = {
      description = "A tiny C++11 test framework – lest errors escape testing.";
      homepage = "https://github.com/martinmoene/lest";
      license = pkgs.lib.licenses.boost;
    };

    src = fetchGit {
      url = "https://github.com/martinmoene/lest.git";
      rev = "1eda2f7c33941617fc368ce764b5fbeffccb59bc";
    };

    postBuild = ''
      mkdir -p "$out/include"
      cp -r include "$out/"
    '';

  };

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
