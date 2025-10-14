{ withCcache ? false,

  doCheck ? true, # boot unikernel after building it
  includeos ? import ./default.nix { inherit withCcache; },
}:

includeos.stdenv.mkDerivation rec {
  pname = "includeos_example";
  src = includeos.pkgs.lib.cleanSource ./example;
  dontStrip = true;
  inherit doCheck;

  nativeBuildInputs = [
    includeos.pkgs.buildPackages.nasm
    includeos.pkgs.buildPackages.cmake
  ] ++ [ includeos.pkgs.pkgsIncludeOS.suppressTargetWarningHook ];

  buildInputs = [
    includeos
    includeos.chainloader
  ];

  nativeCheckInputs = [
    includeos.vmrunner
    includeos.pkgs.qemu
  ];

  checkPhase = ''
    boot *.elf.bin
  '';

  version = "dev";
}
