# Nix expression to build IncludeOS.
# See https://nixos.org/nix for info about Nix.
#
# Usage:
#
# $ nix-build ./path/to/this/file.nix
#
# Author: Bjørn Forsman <bjorn.forsman@gmail.com>

{ nixpkgs ?
  builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz";
    sha256 = "1ndiv385w1qyb3b18vw13991fzb9wg4cl21wglk89grsfsnra41k";
  },
  pkgs ? (import nixpkgs { }).pkgsStatic, # should we add pkgsStatic here? Fails on cmake
  stdenv ? pkgs.llvmPackages_7.stdenv
}:

assert (stdenv.buildPlatform.isLinux == false) ->
  throw "Currently only Linux builds are supported";
assert (stdenv.hostPlatform.isMusl == false) ->
  throw "Stdenv should be based on Musl";

let

  musl-includeos = pkgs.callPackage ./deps/musl/default.nix { inherit nixpkgs stdenv pkgs; };
  uzlib = pkgs.callPackage ./deps/uzlib/default.nix { inherit stdenv pkgs; };
  botan2 = pkgs.callPackage ./deps/botan/default.nix { inherit pkgs; };
  microsoft_gsl = pkgs.callPackage ./deps/GSL/default.nix { inherit stdenv; };
  s2n-tls = pkgs.callPackage ./deps/s2n/default.nix { inherit stdenv pkgs; };
  http-parser = pkgs.callPackage ./deps/http-parser/default.nix { inherit stdenv; };

  # pkgs.cmake fails on unknown argument --disable-shared. Override configurePhase to not use the flag.
  cmake = pkgs.cmake.overrideAttrs(oldAttrs: {
    inherit stdenv;
    useSharedLibraries=false;
    isMinimalBuild=true;
    # Override configure phase, otherwise it will fail on unsupported flags,.
    # Add some manual flags taken from cmake.nix.
    configurePhase = ''
      ./configure --prefix=$out --parallel=''${NIX_BUILD_CORES:-1} CC=$CC_FOR_BUILD CXX=$CXX_FOR_BUILD CXXFLAGS=-Wno-elaborated-enum-base --no-system-libs
    '';
  });

  includeos = stdenv.mkDerivation rec {
    pname = "includeos";

    version = "dev";

    src = pkgs.lib.cleanSource ./.;

    # If you need to patch, this is the place
    postPatch = '''';

    nativeBuildInputs = [
      cmake
      pkgs.nasm
    ];

    buildInputs = [
      musl-includeos
      botan2
      http-parser
      microsoft_gsl
      pkgs.openssl
      pkgs.rapidjson
      #s2n-tls
      uzlib
    ];

    # Add some pasthroughs, for easily building the depdencies (for debugging):
    # $ nix-build -A NAME
    passthru = {
      inherit uzlib;
      inherit http-parser;
      inherit botan2;
      #inherit s2n-tls;
      inherit musl-includeos;
    };

    meta = {
      description = "Run your application with zero overhead";
      homepage = "https://www.includeos.org/";
      license = pkgs.lib.licenses.asl20;
    };
  };
in
  includeos
