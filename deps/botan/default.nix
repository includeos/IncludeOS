{
  pkgs
}:
let
  cpuFlag = if pkgs.stdenv.system == "i686-linux" then "x86_32" else "x86_64";
in
pkgs.botan2.overrideAttrs (oldAttrs: {

  postInstall = (oldAttrs.postInstall or "") + ''
    ln -sr "$out/include/botan-2/botan" "$out/include"
  '';

  configurePhase = ''
    runHook preConfigure
    python3 configure.py --prefix=$out --with-bzip2 --with-zlib --build-targets=static --cpu=${cpuFlag} --os=linux
    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild

    make -j $NIX_BUILD_CORES

    runHook postBuild
  '';
})
