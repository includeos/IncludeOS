{
stdenv
, pkgs
, linuxHeaders ? null
}:
stdenv.mkDerivation rec {
  pname = "musl-unpatched";
  version = "1.1.24";

  src = fetchGit {
    url = "git://git.musl-libc.org/musl";
    rev = "ea9525c8bcf6170df59364c4bcd616de1acf8703";
  };

  enableParallelBuilding = true;

  configurePhase = ''
    echo "Configuring with musl's configure script"
    echo "Target platform is ${stdenv.targetPlatform.config}"
    ./configure --prefix=$out --disable-shared --enable-debug CROSS_COMPILE=${stdenv.targetPlatform.config}-
  '';

  # Copy linux headers - taken from upstream nixpkgs musl, needed for libcxx to build
  postInstall = ''
    (cd $out/include && ln -s $(ls -d ${linuxHeaders}/include/* | grep -v "scsi$") .)
  '';

  CFLAGS = "-Wno-error=int-conversion -nostdinc";

  passthru.linuxHeaders = linuxHeaders;

  meta = {
    description = "musl - Linux based libc (unpatched)";
    homepage = "https://www.musl-libc.org/";
    license = pkgs.lib.licenses.mit;
  };
}
