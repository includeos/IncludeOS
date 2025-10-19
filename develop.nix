# develop.nix
# sets up a development shell in which you can open your editor
{
  buildpath ? "build",

  # path to your unikernel source (project root with CMakeLists.txt)
  unikernel ? ".",

  # optional: path to a local vmrunner checkout (otherwise use includeos.vmrunner)
  vmrunner ? "",

  # Enable ccache support. See overlay.nix for details.
  withCcache ? true,

  # Enable multicore suport.
  smp ? false,

  includeos ? import ./default.nix { inherit withCcache smp; },

  arch ? "x86_64"
}:

# override stdenv for furhter derivations so they're in sync with includeos patch requirements
includeos.pkgs.mkShell.override { inherit (includeos) stdenv; } rec {
  vmrunnerPkg =
    if vmrunner == "" then
      includeos.vmrunner
    else
      includeos.pkgs.callPackage (builtins.toPath /. + vmrunner) {};

  # handy tools available in the shell
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
    includeos.pkgs.jq
  ];

  # libraries/headers we include against
  buildInputs = [
    includeos
    includeos.chainloader
    includeos.lest
    includeos.pkgs.openssl
    includeos.pkgs.rapidjson
  ];

  shellHook = ''
    IOS_SRC=${toString ./.}
    if [ ! -d "$IOS_SRC" ]; then
        echo "$unikernel is not a valid directory" >&2
        return 1
    fi

    echo "Configuring in: ${buildpath}"
    echo "Source tree: $IOS_SRC"

    # delete old just in case it's dirty
    rm -rf ${buildpath}
    mkdir -p ${buildpath}

    # build includeOS
    cmake -S "$IOS_SRC" -B ${buildpath} \
      -D CMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -D ARCH=${arch} \
      -D CMAKE_MODULE_PATH=${includeos}/cmake

    # procuced by CMake
    CCDB="${buildpath}/compile_commands.json"

    #
    # attempting to use -resource-dir with 'clang++ -print-resource-dir'
    # doesn't work here as we're using -nostdlib/-nostdlibinc
    #
    tmp="$CCDB.clangd.tmp"
    jq \
      --arg libcxx "${includeos.libraries.libcxx.include}" \
      --arg libc "${includeos.libraries.libc}"             \
      --arg libfmt "${includeos.passthru.libfmt.include}"  \
      --arg localsrc "${toString ./.}"                     \
      '
      map(.command |= ( .
          + " -isystem \($libcxx)"
          + " -isystem \($libc)/include"
          + " -I \($libfmt)"
          | gsub("(?<a>-I)(?<b>/lib/LiveUpdate/include)"; .a + $localsrc + .b)
      ))
    ' "$CCDB" > "$tmp" && mv "$tmp" "$CCDB"


      # most clangd configurations and editors will look in ./build/, but this just makes it easier to find for some niche edge cases
      ln -sfn "${buildpath}/compile_commands.json" "$IOS_SRC/compile_commands.json"
  '';
}

