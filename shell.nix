{
  # Will create a temp one if none is passed, for example:
  # nix-shell --argstr buildpath .
  buildpath ? "",

  # vmrunner path, for vmrunner development
  vmrunner ? "",

  # Enable ccache support. See overlay.nix for details.
  withCcache ? false,

  # Enable multicore suport.
  smp ? false,

  includeos ? import ./default.nix { inherit withCcache; inherit smp; }
}:

includeos.pkgs.mkShell.override { inherit (includeos) stdenv; } rec {
  vmrunnerPkg =
    if vmrunner == "" then
      includeos.vmrunner
    else
      includeos.pkgs.callPackage (builtins.toPath /. + vmrunner) {};

  packages = [
    (includeos.pkgs.python3.withPackages (p: [
      vmrunnerPkg
    ]))
    includeos.pkgs.buildPackages.cmake
    includeos.pkgs.buildPackages.nasm
    includeos.pkgs.qemu
    includeos.pkgs.which
    includeos.pkgs.grub2
    includeos.pkgs.iputils
    includeos.pkgs.xorriso
  ];

  buildInputs = [
    includeos
    includeos.chainloader
    includeos.lest
    includeos.pkgs.openssl
    includeos.pkgs.rapidjson
  ];

  shellHook = ''
    echo -e "\n====================== IncludeOS nix-shell ====================="
    echo -e "\nThe C++ compiler set to:"
    echo $(which $CXX)
    echo -e "\nIncludeOS package:"
    echo ${includeos}
    echo -e "\n---------------------- Network privileges  ---------------------"
    echo "The vmrunner for IncludeOS tests requires bridged networking for full functionality."
    echo "The following commands requiring sudo privileges can be used to set this up:"
    echo "1. the qemu-bridge-helper needs sudo to create a bridge. Can be enabled with:"
    echo "   sudo chmod u+s ${includeos.pkgs.qemu}/libexec/qemu-bridge-helper"
    echo "2. bridge43 must exist. Can be set up with vmrunner's create_bridge.sh script:"
    echo "   ${vmrunnerPkg.create_bridge}"
    echo "3. /etc/qemu/bridge.conf must contain this line:"
    echo "   allow bridge43"
    echo ""
    echo "Some tests require ping, which requires premissions to send raw packets. On some hosts"
    echo "this is not enabled by default for iputils provided by nix. It can be enabled with:"
    echo "4. sudo setcap cap_net_raw+ep ${includeos.pkgs.iputils}/bin/ping"
    echo " "
    echo
  '';
}
