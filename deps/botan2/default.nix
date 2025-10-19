{
  pkgs
}:
let
  cpuFlag = if pkgs.stdenv.system == "i686-linux" then "x86_32" else "x86_64";

  self = pkgs.botan2;

  dev = pkgs.libs.getDev self;
  lib = pkgs.libs.getLib self;
in
  self.overrideAttrs (oldAttrs: {

    postInstall = (oldAttrs.postInstall or "") + ''
      ln -sr "$out/include/botan-2/botan" "$out/include"
    '';

    buildPhase = ''
      runHook preBuild

      make -j $NIX_BUILD_CORES

      runHook postBuild
    '';
  })

  passthru = (oldAttrs.passthru or {}) // {
    include_root = "${dev}/include";
    include = "${dev}/include/botan-2";  # include/botan2/botan/*.h
    lib = "${lib}/lib";
  };
})
