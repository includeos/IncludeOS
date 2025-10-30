{
  pkgs,
  stdenv
}:
let
  self = stdenv.mkDerivation rec {
    pname = "lest";
    version = "1.36.0";

    meta = {
      description = "A tiny C++11 test framework – lest errors escape testing.";
      homepage = "https://github.com/martinmoene/lest";
      license = pkgs.lib.licenses.boost;
    };

    src = fetchGit {
      url = "https://github.com/martinmoene/lest.git";
      ref = "refs/tags/v${version}";
      rev = "57197f32f2c7d3f3d3664a9010d3ff181a40f6ca";
    };

    cmakeBuildType = "Debug";

    postBuild = ''
        mkdir -p "$out/include"
        cp -r include "$out/"
      '';
  };

  dev = pkgs.lib.getDev self;
  lib = pkgs.lib.getLib self;
in
  self.overrideAttrs (prev: {
    passthru = (prev.passthru or {}) // {
      include_root = "${dev}/include";
      include = "${dev}/include/lest";
      # lib = "${self}";  # TODO: consider precompiling
    };
  })
