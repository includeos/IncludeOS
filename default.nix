# Nix expression to build IncludeOS.
# See https://nixos.org/nix for info about Nix.
#
# Usage:
#
# $ nix-build ./path/to/this/file.nix
#
# Authors: Bjørn Forsman <bjorn.forsman@gmail.com>

{ nixpkgs ? <nixpkgs>, # Builds cleanly with unstable, May 9. 2024

  # TODO: We want to pin nixpkgs, but:
  #
  # builtins.fetchTarball {
  #
  # This👇 "is not able to compile a simple test program" (clang via cmake)
  #  url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/24.05-pre.tar.gz";
  #  sha256 = "1cfbkahcfj1hgh4v5nfqwivg69zks8d72n11m5513i0phkqwqcgh";

  # This👇 "is not able to compile a simple test program" (clang via cmake)
  # url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz";
  # sha256 = "1ndiv385w1qyb3b18vw13991fzb9wg4cl21wglk89grsfsnra41k";
  #
  #},

  pkgs ? (import nixpkgs { }).pkgsStatic,

  # This env has musl and LLVM's libc++ as static libraries.
  stdenv ? pkgs.llvmPackages_16.libcxxStdenv
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

      # TODO:
      # including musl here makes the compiler pick up musl's libc headers
      # before libc++'s libc headers, which doesn't work. See e.g. <cstdint>
      # line 149;
      # error <cstdint> tried including <stdint.h> but didn't find libc++'s <stdint.h> header.
      # #ifndef _LIBCPP_STDINT_H
      #   error <cstdint> tried including <stdint.h> but didn't find libc++'s <stdint.h> header. \
      #   This usually means that your header search paths are not configured properly. \
      #   The header search paths should contain the C++ Standard Library headers before \
      #   any C Standard Library, and you are probably using compiler flags that make that \
      #   not be the case.
      # #endif
      #
      # With this commented out IncludeOS builds with Nix libc++, which is great,
      # but might bite us later because it then uses a libc we didn't patch.
      # Best case we case we can adapt our own musl to be identical, use nix static
      # musl for compilation, and includeos-musl only for linking.
      #
      # musl-includeos  👈 this has to come in after libc++ headers.
      botan2
      http-parser
      microsoft_gsl
      pkgs.openssl
      pkgs.rapidjson
      #s2n-tls          👈 This is postponed until we can fix the s2n build.
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
