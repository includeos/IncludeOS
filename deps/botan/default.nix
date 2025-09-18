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

  buildPhase = ''
    runHook preBuild

    make -j $NIX_BUILD_CORES

    runHook postBuild
  '';
})
