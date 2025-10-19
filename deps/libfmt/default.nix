# note that there is also `<nixpkgs>.fmt`
# so this entire file could just be `{ pkgs }: pkgs.fmt`
{
  pkgs,
  stdenv ? pkgs.stdenv,
  cmake ? pkgs.cmake
}:
let
  libfmt = stdenv.mkDerivation rec {
    pname = "fmt";
    version = "12.0.0";

    src = pkgs.fetchFromGitHub {
      owner = "fmtlib";
      repo  = "fmt";
      rev   = "12.0.0";
      hash = "sha256-AZDmIeU1HbadC+K0TIAGogvVnxt0oE9U6ocpawIgl6g=";
    };

    nativeBuildInputs = [ cmake ];

    cmakeFlags = [
      "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY"
      "-DBUILD_SHARED_LIBS=OFF"

      "-DFMT_TEST=OFF"
      "-DFMT_DOC=OFF"
      "-DFMT_INSTALL=ON"
    ];
  };
in
  libfmt // {
    include = "${libfmt}/include";
    lib = "${libfmt}/lib";
  }
