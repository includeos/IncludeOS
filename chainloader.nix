{ nixpkgs ? ./pinned.nix,
  overlays ? [
    (import ./overlay.nix)
  ],
  pkgs ? import nixpkgs {
      config = { };
      inherit overlays;
      crossSystem = {
        config = "i686-unknown-linux-musl";
      };
  },
  llvmPkgs ? pkgs.llvmPackages_16
}:
let
  includeos = pkgs.pkgsIncludeOS.includeos;
in

assert (includeos.stdenv.targetPlatform.system != "i686-linux") ->
  throw "Chainloader must be built as 32-bit target";
assert (includeos.stdenv.targetPlatform.isLinux == false) ->
  throw "Target platform must be Linux";
assert (includeos.stdenv.targetPlatform.isMusl == false) ->
  throw "Target stdenv should be based on Musl";

includeos.stdenv.mkDerivation rec {
  pname = "chainloader";
  version = "dev";

  sourceRoot = "./src/chainload/";
  hardeningDisable = [ "pie" ]; # use "all" to disable all hardening options

  libcxx      = "${includeos.stdenv.cc.libcxx}/lib/libc++.a";
  libcxxabi   = "${includeos.stdenv.cc.libcxx}/lib/libc++abi.a";
  libunwind   = "${llvmPkgs.libraries.libunwind}/lib/libunwind.a";
  compiler-rt = "${llvmPkgs.compiler-rt}/lib/linux/libclang_rt.builtins-i386.a";

  linkdeps = [
    libcxx
    libcxxabi
    libunwind
  ];

  cmakeFlags = [
    "-DINCLUDEOS_PACKAGE=${includeos}"
    "-DINCLUDEOS_LIBC_PATH=${includeos.musl-includeos}/lib/libc.a"
    "-DINCLUDEOS_LIBCXX_PATH=${libcxx}"
    "-DINCLUDEOS_LIBCXXABI_PATH=${libcxxabi}"
    "-DINCLUDEOS_LIBUNWIND_PATH=${libunwind}"
    "-DINCLUDEOS_LIBGCC_PATH=${compiler-rt}"
  ];

  srcs = [
    ./src
    ./api
    ./cmake
    ];

  nativeBuildInputs = [
    pkgs.cmake
    pkgs.nasm
  ];

  buildInputs = [
    pkgs.microsoft_gsl
    pkgs.pkgsStatic.llvmPackages_16.compiler-rt
  ];

}
