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
    cat <<-EOF
================================== IncludeOS nix-shell ==================================
Packages:
  IncludeOS:       ${includeos}
  vmrunner:        ${vmrunnerPkg}
  chainloader:     ${includeos.chainloader}

Tooling:
  CXX              $(command -v $CXX)
  cmake:           $(command -v cmake)
  nasm:            $(command -v nasm)
  qemu-system-x86: $(command -v qemu-system-x86_64)
  grub-mkrescue:   $(command -v grub-mkrescue)
  xorriso:         $(command -v xorriso)
  ping:            $(command -v ping)

---------------------------------- Network privileges  ----------------------------------
The vmrunner for IncludeOS tests requires bridged networking for full functionality.
The following checklist can be used to set this up from the host:

1. The qemu-bridge-helper needs root escalation to manipulate bridges. You can provide this
   either through capabilities or through root execution. Pick one:
       sudo chmod u+s ${includeos.pkgs.qemu}/libexec/qemu-bridge-helper
       sudo setcap cap_net_admin+ep ${includeos.pkgs.qemu}/libexec/qemu-bridge-helper

2. bridge43 must exist. Can be set up with vmrunner's create_bridge.sh script (not as root):
       ${vmrunnerPkg.create_bridge}

3. /etc/qemu/bridge.conf must contain this line:
       allow bridge43
   Also note that /etc/qemu needs specific permissions, so it might be easiest to install
   qemu on the host to generate these directories for you, despite not using its executable here.

4. Some tests also perform ICMP pings, which requires permissions to send raw packets. On some
   hosts this is not enabled by default for iputils provided by nix.
   It can be enabled with:
       sudo setcap cap_net_raw+ep ${includeos.pkgs.iputils}/bin/ping

EOF
  '';
}
