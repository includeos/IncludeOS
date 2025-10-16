{

  # The unikernel to build
  unikernel ? ./example,

  # Boot unikernel after building it
  doCheck ? true,

  # Which architecture to build against
  arch ? "x86_64",

  # Enable multicore suport.
  smp ? false,

  # Enable ccache support. See overlay.nix for details.
  withCcache ? false,

  # Enable stricter requirements
  forProduction ? false,

  # The includeos library to build and link against
  includeos ? import ./default.nix { inherit withCcache; inherit smp; },
}:

includeos.stdenv.mkDerivation rec {
  pname = "includeos_example";
  version = "dev";
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
    "-DARCH=${arch}"
    "-DINCLUDEOS_PACKAGE=${includeos}"
    "-DCMAKE_MODULE_PATH=${includeos}/cmake"
    "-DFOR_PRODUCTION=${if forProduction then "ON" else "OFF"}"
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
}
