# Nix expression to build IncludeOS.
# See https://nixos.org/nix for info about Nix.
#
# Usage:
#
# $ nix-build ./path/to/this/file.nix
#
# Authors: Bjørn Forsman <bjorn.forsman@gmail.com>

{ nixpkgs ? ./pinned.nix, # Builds cleanly May 9. 2024
  pkgs ? (import nixpkgs { }),

  # This env has musl and LLVM's libc++ as static libraries.
  stdenv ? pkgs.pkgsStatic.llvmPackages_16.libcxxStdenv
}:

assert (stdenv.buildPlatform.isLinux == false) ->
  throw "Currently only Linux builds are supported";
assert (stdenv.hostPlatform.isMusl == false) ->
  throw "Stdenv should be based on Musl";

let

  # Static libraries, for linking into IncludeOS bootables.
  # Anything that goes into IncludeOS needs to be staically linkable.
  static = pkgs.pkgsStatic;
  musl-includeos = static.callPackage ./deps/musl/default.nix { inherit nixpkgs stdenv pkgs; };
  uzlib = static.callPackage ./deps/uzlib/default.nix { inherit stdenv pkgs; };
  botan2 = static.callPackage ./deps/botan/default.nix { inherit pkgs; };
  microsoft_gsl = static.callPackage ./deps/GSL/default.nix { inherit stdenv; };
  s2n-tls = static.callPackage ./deps/s2n/default.nix { inherit stdenv pkgs; };
  http-parser = static.callPackage ./deps/http-parser/default.nix { inherit stdenv; };
  
  # We can build the tools with any vanilla C++ environment.
  tools = pkgs.callPackage ./tools/default.nix {};

  includeos = stdenv.mkDerivation rec {
    pname = "includeos";

    version = "dev";

    src = pkgs.lib.cleanSource ./.;

    # If you need to patch, this is the place
    postPatch = '''';

    nativeBuildInputs = [
      pkgs.cmake
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
      inherit tools;
    };

    meta = {
      description = "Run your application with zero overhead";
      homepage = "https://www.includeos.org/";
      license = pkgs.lib.licenses.asl20;
    };
  };
in
  includeos
