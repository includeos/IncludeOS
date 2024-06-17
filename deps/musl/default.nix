{
stdenv
, pkgs
, linuxHeaders ? null
}:
stdenv.mkDerivation rec {
  pname = "musl-includeos";
  version = "1.1.24";

  src = fetchGit {
    url = "git://git.musl-libc.org/musl";
    rev = "ea9525c8bcf6170df59364c4bcd616de1acf8703";
  };

  enableParallelBuilding = true;

  patches = [
    ./patches/musl.patch
    ./patches/endian.patch
  ];

  passthru.linuxHeaders = linuxHeaders;

  postUnpack = ''
    echo "Replacing musl's syscall headers with IncludeOS syscalls"

    cp ${./patches/includeos_syscalls.h} $sourceRoot/src/internal/includeos_syscalls.h
    cp ${./patches/syscall.h} $sourceRoot/src/internal/syscall.h

    rm $sourceRoot/arch/x86_64/syscall_arch.h
    rm $sourceRoot/arch/i386/syscall_arch.h
  '';

 configurePhase = ''
    echo "Configuring with musl's configure script"
    echo "Target platform is ${stdenv.targetPlatform.config}"
    ./configure --prefix=$out --disable-shared --enable-debug CROSS_COMPILE=${stdenv.targetPlatform.config}-
  '';

  CFLAGS = "-Wno-error=int-conversion -nostdinc";

  meta = {
    description = "musl - Linux based libc, built with IncludeOS linux-like syscalls";
    homepage = "https://www.musl-libc.org/";
    license = pkgs.lib.licenses.mit;
  };
}
