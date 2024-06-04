{ nixpkgs ? ./pinned.nix,
  includeos ? import ./default.nix { },
  pkgs ? (import nixpkgs { }).pkgsStatic,
  llvmPkgs ? pkgs.llvmPackages_16
}:

includeos.stdenv.mkDerivation rec {
  pname = "includeos_example";
  src = pkgs.lib.cleanSource ./example;
  doCheck = false;
  dontStrip = true;

  nativeBuildInputs = [
    pkgs.buildPackages.nasm
    pkgs.buildPackages.cmake
  ];

  buildInputs = [
    pkgs.microsoft_gsl
    includeos
  ];

  # TODO:
  # We currently need to explicitly pass in because we link with a linker script
  # and need to control linking order.
  # This can be moved to os.cmake eventually, once we figure out how to expose
  # them to cmake from nix without having to make cmake depend on nix.
  # * Maybe we should make symlinks from the includeos package to them.

  libcxx    = "${includeos.stdenv.cc.libcxx}/lib/libc++.a";
  libcxxabi = "${includeos.stdenv.cc.libcxx}/lib/libc++abi.a";
  libunwind = "${llvmPkgs.libraries.libunwind}/lib/libunwind.a";

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

    "-DARCH=x86_64"
    "-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON"
  ];

  preBuild = ''
    echo ""
    echo "📣 preBuild: about to build - can it work?  Yes! 🥁🥁🥁"
    echo "Validating dependencies: "
    for dep in ${toString linkdeps}; do
        echo "Checking $dep:"
        file $dep
      done
    echo ""
  '';

  postBuild = ''
    echo "🎉 POST BUILD - you made it pretty far! 🗻⛅"
    if [[ $? -ne 0 ]]; then
      echo "Build failed. Running post-processing..."
      echo "Performing cleanup or logging"
    fi
  '';

  version = "dev";
}
