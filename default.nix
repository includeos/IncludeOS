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
  }
, pkgs ? (import nixpkgs { }).pkgs # should we add pkgsStatic here? Fails on cmake
}:

with pkgs;

let
  stdenv = makeStaticLibraries pkgs.llvmPackages_7.libcxxStdenv; # could also use makeStatic?

  musl-includeos = callPackage ./deps/musl/default.nix { inherit nixpkgs pkgs stdenv; };
  uzlib = callPackage ./deps/uzlib/default.nix { inherit stdenv pkgs; };
  botan2 = callPackage ./deps/botan/default.nix { inherit pkgs; };
  microsoft_gsl = callPackage ./deps/GSL/default.nix { inherit stdenv; };
  s2n-tls = callPackage ./deps/s2n/default.nix { inherit stdenv pkgs; };

  # Add needed $out/include/http-parser directory to match IncludeOS' use of
  # "#include <http-parser/http_parser.h>".
  # TODO: Upstream doesn't use that subdir though, so better fix IncludeOS
  # sources.
  #http-parser = pkgs.pkgsStatic.http-parser.overrideAttrs (oldAttrs: {
    #postInstall = (oldAttrs.postInstall or "") + ''
      #mkdir "$out/include/http-parser"
      #ln -sr "$out/include/http_parser.h" "$out/include/http-parser"
    #'';
  #});

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
      musl-includeos
      botan2
      #http-parser
      microsoft_gsl
     # openssl
     # rapidjson
      s2n-tls
      uzlib
    ];

    # Add some pasthroughs, for easily building the depdencies (for debugging):
    # $ nix-build -A NAME
    passthru = {
      inherit uzlib;
      #inherit http-parser;
      inherit botan2;
      inherit s2n-tls;
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
