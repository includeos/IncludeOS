# Nix expression to build IncludeOS.
# See https://nixos.org/nix for info about Nix.
#
# Usage:
#
# $ nix-build ./path/to/this/file.nix
#
# Author: Bjørn Forsman <bjorn.forsman@gmail.com>

{ nixpkgs ?
  builtins.fetchTarball "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz"

, pkgs ? import nixpkgs { config = {}; overlays = []; }
}:

with pkgs;

let
  stdenv = clang7Stdenv;

  # Get older dependencies from here
  # TODO: Looks like it's only GSL 4.0.0 we're choking on so we can probably just update that.
  nix_20_09 = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/38eaa62f28384bc5f6c394e2a99bd6a4913fc71f.tar.gz";
  }) {};


  uzlib = stdenv.mkDerivation rec {
    pname = "uzlib";

    # Latest version, seems incompatible with IncludeOS.
    #version = "2.9.5";
    #
    #src = fetchzip {
    #  url = "https://github.com/pfalcon/uzlib/archive/v${version}.tar.gz";
    #  sha256 = "01l5y3rwa9935bqlrgww71zr83mbdinq69xzk2gfk96adgjvrl7k";
    #};

    # same version as listed in ./conanfile.py
    version = "2.1.1";

    src = fetchzip {
      url = "https://github.com/pfalcon/uzlib/archive/v${version}.tar.gz";
      sha256 = "1bdbfkxq648blh6v7lvvy1dhrykmib1kzpgjh1fb5zhzq5xib9b2";
    };

    # v2.1.1 has no top-level Makefile
    buildPhase = ''
      make -C src -f makefile.elf
    '';

    postPatch = ''
      echo 'Replacing gcc with $(CC) in makefile.elf'
      sed 's/gcc/$(CC)/g' -i ./src/makefile.elf
    '';

    # Upstream doesn't have an install target (not even in the latest version)
    installPhase = ''
      runHook preInstall

      #ls -lR

      mkdir -p "$out/include"
      cp src/tinf.h "$out/include"
      #cp src/tinf_compat.h "$out/include"  # doesn't exist in v2.1.1
      #cp src/uzlib.h "$out/include"  # doesn't exist in v2.1.1
      cp src/defl_static.h "$out/include"
      #cp src/uzlib_conf.h "$out/include"  # doesn't exist in v2.1.1

      mkdir -p "$out/lib"
      cp lib/libtinf.a "$out/lib"

      runHook postInstall
    '';

    meta = {
      description = "Radically unbloated DEFLATE/zlib/gzip compression/decompression library";
      homepage = "https://github.com/pfalcon/uzlib";
      license = lib.licenses.zlib;
    };
  };

  # Add needed $out/include/http-parser directory to match IncludeOS' use of
  # "#include <http-parser/http_parser.h>".
  # TODO: Upstream doesn't use that subdir though, so better fix IncludeOS
  # sources.
  http-parser = pkgs.http-parser.overrideAttrs (oldAttrs: {
    postInstall = (oldAttrs.postInstall or "") + ''
      mkdir "$out/include/http-parser"
      ln -sr "$out/include/http_parser.h" "$out/include/http-parser"
    '';
  });

  # TODO: which code base to fix, includeos or nixpkgs?
  botan2 = pkgs.botan2.overrideAttrs (oldAttrs: {
    postInstall = (oldAttrs.postInstall or "") + ''
      ln -sr "$out/include/botan-2/botan" "$out/include"
    '';
  });

  s2n-tls = stdenv.mkDerivation rec {
    pname = "s2n-tls";
    # ./conanfile.py lists 0.8, but there are not tags in the repo with version < 0.9.0
    version = "0.9.0";

    src = fetchzip {
      url = "https://github.com/aws/s2n-tls/archive/v${version}.tar.gz";
      sha256 = "18qjqc2jrpiwdpzqxl6hl1cq0nfmqk8qas0ijpwr0g606av0aqm9";
    };

    buildInputs = [
      openssl
    ];

    # the default 'all' target depends on tests which are broken (see below)
    buildPhase = ''
      runHook preBuild

      make bin

      runHook postBuild
    '';

    # TODO: tests fail:
    # make -C unit
    # make[2]: Entering directory '/build/source/tests/unit'
    # Running s2n_3des_test.c                                    ... FAILED test 1
    # !((conn = s2n_connection_new(S2N_SERVER)) == (((void *)0))) is not true  (s2n_3des_test.c line 44)
    # Error Message: 'error calling mlock (Did you run prlimit?)'
    #  Debug String: 'Error encountered in s2n_mem.c line 103'
    # make[2]: *** [Makefile:44: s2n_3des_test] Error 1
    doCheck = false;

    # Upstream Makefile has no install target
    installPhase = ''
      runHook preInstall

      mkdir -p "$out/include"
      cp api/s2n.h "$out/include"

      mkdir -p "$out/lib"
      cp lib/libs2n.a lib/libs2n.so "$out/lib"

      runHook postInstall
    '';

    meta = {
      description = "An implementation of the TLS/SSL protocols";
      homepage = "https://github.com/aws/s2n-tls";
      license = lib.licenses.asl20;
    };
  };

  includeos = stdenv.mkDerivation rec {
    pname = "includeos";

    version = "dev";

    src = lib.cleanSource ./.;

    # * Disable conan from CMake files
    # * REVISIT: Add #include <linux/limits.h>, <bits/xopen_lim.h> to fix missing IOV_MAX macro
    # * Add missing #include <assert.h>
    # * Disable conan in cmake/os.cmake
    # * Remove -march=native impurity (better tell the system what to build
    #   than to get whatever the current build machine is)
    postPatch = ''
      echo "Disabling conan from CMake files"
      patch -p1 <<EOF
      diff --git a/CMakeLists.txt b/CMakeLists.txt
      index 82aff91a3..4764de894 100644
      --- a/CMakeLists.txt
      +++ b/CMakeLists.txt
      @@ -7,6 +7,7 @@ project (includeos C CXX)

       option(PROFILE "Compile with startup profilers" OFF)

      +if(FALSE)  # disable conan
       #Are we executing cmake from conan or locally
       #if locally then pull the deps from conanfile.py
       #if buiding from conan expect the conanbuildinfo.cmake to already be present
      @@ -54,6 +55,7 @@ else() # in user space
           ''${CONANPROFILE}
         )
       endif()
      +endif()

       include(cmake/includeos.cmake)
      EOF

      echo "Adding #include <linux/limits.h>"
      patch -p1 <<EOF
      diff --git a/src/musl/fstatat.cpp b/src/musl/fstatat.cpp
      index 24434bcde..09fb591e3 100644
      --- a/src/musl/fstatat.cpp
      +++ b/src/musl/fstatat.cpp
      @@ -1,5 +1,6 @@
       #include "common.hpp"
       #include <sys/stat.h>
      +#include <linux/limits.h>

       #include <posix/fd_map.hpp>

      EOF

      echo "Adding #include <bits/xopen_lim.h>"
      patch -p1 <<EOF
      diff --git a/src/posix/file_fd.cpp b/src/posix/file_fd.cpp
      index eb00dc4ae..f44d56c96 100644
      --- a/src/posix/file_fd.cpp
      +++ b/src/posix/file_fd.cpp
      @@ -3,6 +3,7 @@
       #include <errno.h>
       #include <dirent.h>
       #include <sys/uio.h>
      +#include <bits/xopen_lim.h>

       ssize_t File_FD::read(void* p, size_t n)
       {
      EOF

      echo "Adding missing #include <assert.h>"
      grep -rnl --include="*.hpp" --include="*.cpp" "\<assert(" | while read f; do
          if ! grep -q "#include .*assert\.h" "$f"; then
              echo "inserting missing #include <assert.h>: $f"
              sed -i -e "1i #include <assert.h>" "$f"
          fi
      done

      echo "Disabling conan in cmake/os.cmake"
      sed -e "s/conan_find_libraries_abs_path/#conan_find_libraries_abs_path/g" \
          -i cmake/os.cmake

      echo "Remove -march=native impurity from CMake files"
      grep -rnl march=native . | while read -r f; do
          sed -e "s/-march=native//g" -i "$f"
      done
    '';

    nativeBuildInputs = [
      cmake
      nasm
    ];

    buildInputs = [
      botan2
      http-parser
      nix_20_09.microsoft_gsl
      openssl
      rapidjson
      s2n-tls
      uzlib
    ];

    # Add some pasthroughs, for easily building the depdencies (for debugging):
    # $ nix-build -A NAME
    passthru = {
      inherit uzlib;
      inherit http-parser;
      inherit botan2;
      inherit s2n-tls;
    };

    meta = {
      description = "Run your application with zero overhead";
      homepage = "https://www.includeos.org/";
      license = lib.licenses.asl20;
    };
  };
in
  includeos
