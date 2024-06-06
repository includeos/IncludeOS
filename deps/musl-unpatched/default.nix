{ nixpkgs ?
  builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz";
    sha256 = "1ndiv385w1qyb3b18vw13991fzb9wg4cl21wglk89grsfsnra41k";
  }
, stdenv
, pkgs ? import nixpkgs { config = {}; overlays = []; crossSystem = { config = stdenv.targetPlatform.config; }; }
, linuxHeaders ? null
}:
stdenv.mkDerivation rec {
  pname = "musl-unpatched";
  version = "1.1.18";

  hardeningDisable = [ "pie" "relro" "stackprotector" ];

  src = fetchGit {
    url = "git://git.musl-libc.org/musl";
    rev = "eb03bde2f24582874cb72b56c7811bf51da0c817";
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
