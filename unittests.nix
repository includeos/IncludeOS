{ nixpkgs ? ./pinned.nix,
  pkgs ? import nixpkgs { config = { }; overlays = [ ]; },
  stdenv ? pkgs.llvmPackages_20.libcxxStdenv,
  withCcache ? false,
}:
stdenv.mkDerivation rec {
  pname = "unittests";
  version = "dev";
  enableParallelBuilding = true;

  sourceRoot = "test";

  ccacheWrapper = pkgs.ccacheWrapper.override {
    inherit (stdenv) cc;
    extraConfig = ''
      export CCACHE_COMPRESS=1
      export CCACHE_DIR="/nix/var/cache/ccache"
      export CCACHE_UMASK=007
      export CCACHE_SLOPPINESS=random_seed
      if [ ! -d "$CCACHE_DIR" ]; then
        echo "====="
        echo "Directory '$CCACHE_DIR' does not exist"
        echo "Please create it with:"
        echo "  sudo mkdir -m0770 '$CCACHE_DIR'"
        echo "  sudo chown root:nixbld '$CCACHE_DIR'"
        echo "====="
        exit 1
      fi
      if [ ! -w "$CCACHE_DIR" ]; then
        echo "====="
        echo "Directory '$CCACHE_DIR' is not accessible for user $(whoami)"
        echo "Please verify its access permissions"
        echo "====="
        exit 1
      fi
    '';
  };

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
  ] ++ pkgs.lib.optionals withCcache [ccacheWrapper];

  buildInputs = [
    pkgs.rapidjson
    pkgs.http-parser
    pkgs.openssl
    lest
    uzlib
    libfmt
  ];
}
