# mkUnikernel.nix
{ system, default, vmrunner }:
args:
let
  ios           = if args ? includeos then args.includeos else default;
  vmrunnerPkg   = if args ? vmrunner then args.vmrunner
                  else vmrunner.packages.${system}.default;

  unikernel     = args.unikernel or ./example;
  test          = args.test or "test.py";
  doCheck       = args.doCheck or false;
  arch          = args.arch or "x86_64";
  forProduction = args.forProduction or false;

  src = unikernel;
in
ios.includeos.stdenv.mkDerivation {
  pname = "includeos_unikernel";
  version = "dev";
  dontStrip = true;
  inherit doCheck;
  inherit src;

  nativeBuildInputs = [
    ios.pkgs.buildPackages.nasm
    ios.pkgs.buildPackages.cmake
    ios.pkgsIncludeOS.suppressTargetWarningHook
  ];

  buildInputs = [
    ios.includeos
    ios.chainloader
  ];

  cmakeFlags = [
    "-DARCH=${arch}"
    "-DINCLUDEOS_PACKAGE=${ios.includeos}"
    "-DCMAKE_MODULE_PATH=${ios.includeos}/cmake"
    "-DFOR_PRODUCTION=${if forProduction then "ON" else "OFF"}"
  ];

  installPhase = ''
    runHook preInstall
    # we want to place any files required by the test into the output
    find -mindepth 1 -maxdepth 1 -type f -exec install -v -D -t "$out/" {} \;

    # especially the unikernel image, in case it wasn't at the rootdir already
    find -mindepth 2 -name '*.elf.bin' -exec install -v -t "$out/" {} \;
    runHook postInstall
  '';

  nativeCheckInputs = [
    vmrunnerPkg
    ios.pkgs.grub2
    ios.pkgs.python3
    ios.pkgs.qemu
    ios.pkgs.iputils
    ios.pkgs.xorriso
  ];

  checkInputs = [
    ios.includeos.lest
  ];

  checkPhase = ''
    runHook preCheck
    set -e
    if [ -e "${src}/${test}" ]; then
      echo "Running IncludeOS test: ${src}/${test}"
      python3 "${src}/${test}"
    else
      echo "Default test script '${test}', but no test was found 😟"
      echo "For a custom path, consider specifying the path to the test script:"
      echo "    --argstr test 'path/to/test.py'"
      exit 1
    fi
    runHook postCheck
  '';
}
