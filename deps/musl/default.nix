{ nixpkgs ?
  builtins.fetchTarball "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz"
, pkgs ? import nixpkgs { config = {}; overlays = []; }
}:
with pkgs;
let
  stdenv = clang7Stdenv;
  musl-includeos=stdenv.mkDerivation rec {
    pname = "musl-includeos";
    version = "1.1.18";

    src = fetchGit {
      url = "git://git.musl-libc.org/musl";
      rev = "eb03bde2f24582874cb72b56c7811bf51da0c817";
    };

    nativeBuildInputs = [ git clang tree];

    patches = [
      ./patches/musl.patch
      ./patches/endian.patch
    ];

    postUnpack = ''
      echo "Replacing musl's syscall headers with IncludeOS syscalls"

      cp ${./patches/includeos_syscalls.h} $sourceRoot/src/internal/includeos_syscalls.h
      cp ${./patches/syscall.h} $sourceRoot/src/internal/syscall.h

      rm $sourceRoot/arch/x86_64/syscall_arch.h
      rm $sourceRoot/arch/i386/syscall_arch.h
      '';

   configurePhase = ''
      echo "Configuring with musl's configure script"
      ./configure --prefix=$out --disable-shared --enable-debug
      '';

    CFLAGS = "-Wno-error=int-conversion -nostdinc";

    meta = {
      description = "musl - an implementation of the standard library for Linux-based systems";
      homepage = "https://www.musl-libc.org/";
      license = lib.licenses.mit;
      maintainers = with lib.maintainers; [ includeos ];
    };
  };
in
musl-includeos
