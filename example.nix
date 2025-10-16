{

  # The unikernel to build
  unikernel ? ./example,

  # Boot unikernel after building it
  doCheck ? true,

  # Enable multicore suport.
  smp ? false,

  # Enable ccache support. See overlay.nix for details.
  withCcache ? false,

  # The includeos library to build and link against
  includeos ? import ./default.nix { inherit withCcache; inherit smp; },
}:

includeos.stdenv.mkDerivation rec {
  pname = "includeos_example";
  src = includeos.pkgs.lib.cleanSource "${unikernel}";
  dontStrip = true;
  inherit doCheck;

  nativeBuildInputs = [
    includeos.pkgs.buildPackages.nasm
    includeos.pkgs.buildPackages.cmake
  ];

  buildInputs = [
    includeos
    includeos.chainloader
  ];

  cmakeFlags = [
    "-DARCH=x86_64"
    "-DINCLUDEOS_PACKAGE=${includeos}"
    "-DCMAKE_MODULE_PATH=${includeos}/cmake"
    "-DFOR_PRODUCTION=OFF"
  ];

  nativeCheckInputs = [
    includeos.vmrunner
    includeos.pkgs.qemu
  ];

  checkPhase = ''
    runHook preCheck
    boot *.elf.bin
    runHook postCheck
  '';

  version = "dev";
}
