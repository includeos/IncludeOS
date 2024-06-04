{
  pkgs,
  stdenv
}:

stdenv.mkDerivation rec {
  pname = "uzlib";

  # Latest version, seems incompatible with IncludeOS.
  #version = "2.9.5";
  #
  #src = fetchzip {
  #  url = "https://github.com/pfalcon/uzlib/archive/v${version}.tar.gz";
  #  sha256 = "01l5y3rwa9935bqlrgww71zr83mbdinq69xzk2gfk96adgjvrl7k";
  #};

  # same version as listed in ./conanfile.py
  version = "2.1.1";

  src = pkgs.fetchzip {
    url = "https://github.com/pfalcon/uzlib/archive/v${version}.tar.gz";
    sha256 = "1bdbfkxq648blh6v7lvvy1dhrykmib1kzpgjh1fb5zhzq5xib9b2";
  };

  # v2.1.1 has no top-level Makefile
  buildPhase = ''
    make -C src -f makefile.elf
  '';

  postPatch = ''
    echo 'Replacing gcc with $(CC) in makefile.elf'
    sed 's/gcc/$(CC)/g' -i ./src/makefile.elf
    sed 's/ar /$(AR) /g' -i ./src/makefile.elf
    sed 's/ranlib /$(RANLIB) /g' -i ./src/makefile.elf
  '';

  # Upstream doesn't have an install target (not even in the latest version)
  installPhase = ''
    runHook preInstall

    #ls -lR

    mkdir -p "$out/include"
    cp src/tinf.h "$out/include"
    #cp src/tinf_compat.h "$out/include"  # doesn't exist in v2.1.1
    #cp src/uzlib.h "$out/include"  # doesn't exist in v2.1.1
    cp src/defl_static.h "$out/include"
    #cp src/uzlib_conf.h "$out/include"  # doesn't exist in v2.1.1

    mkdir -p "$out/lib"
    cp lib/libtinf.a "$out/lib"

    runHook postInstall
  '';

  meta = {
    description = "Radically unbloated DEFLATE/zlib/gzip compression/decompression library";
    homepage = "https://github.com/pfalcon/uzlib";
    license = pkgs.lib.licenses.zlib;
  };
}
