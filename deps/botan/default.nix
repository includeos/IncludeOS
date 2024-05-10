{
  pkgs
}:
pkgs.botan2.overrideAttrs (oldAttrs: {
  postInstall = (oldAttrs.postInstall or "") + ''
    ln -sr "$out/include/botan-2/botan" "$out/include"
  '';

  configurePhase = ''
    runHook preConfigure
    python3 configure.py --prefix=$out --with-bzip2 --with-zlib --build-targets=static
    runHook postConfigure
  '';

  buildPhase = ''
    runHook preBuild

    make

    runHook postBuild
  '';
})
