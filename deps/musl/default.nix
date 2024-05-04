{ nixpkgs ?
  builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz";
    sha256 = "1ndiv385w1qyb3b18vw13991fzb9wg4cl21wglk89grsfsnra41k";
  }
, pkgs ? import nixpkgs { config = {}; overlays = []; }
, stdenv
}:
stdenv.mkDerivation rec {
  pname = "musl-includeos";
  version = "1.1.18";

  src = fetchGit {
    url = "git://git.musl-libc.org/musl";
    rev = "eb03bde2f24582874cb72b56c7811bf51da0c817";
  };

  enableParallelBuilding = true;

  #nativeBuildInputs = [ pkgs.git pkgs.clang pkgs.tree];

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
    ./configure --prefix=$out --disable-shared --enable-debug CROSS_COMPILE=x86_64-unknown-linux-musl-
  '';

  CFLAGS = "-Wno-error=int-conversion -nostdinc";

  meta = {
    description = "musl - Linux based libc, built with IncludeOS linux-like syscalls";
    homepage = "https://www.musl-libc.org/";
    license = pkgs.lib.licenses.mit;
  };
}
