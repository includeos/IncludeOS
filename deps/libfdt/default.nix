{
  pkgs,
  stdenv
}:

let
  self = stdenv.mkDerivation rec {
    pname = "libfdt";
    version = "1.7.2";
    src = pkgs.fetchzip {
      url = "https://mirrors.edge.kernel.org/pub/software/utils/dtc/dtc-${version}.tar.xz";
      hash = "sha256-KZCzrvdWd6zfQHppjyp4XzqNCfH2UnuRneu+BNIRVAY=";
    };
    meta.license = pkgs.lib.licenses.bsd2;

    nativeBuildInputs = with pkgs.buildPackages; [
      pkg-config flex bison
    ];
    outputs = [ "out" "dev" ];

    patches = [
      ./clang-no-suggest-attr-format.patch  # TODO: upstream
    ];

    buildPhase = ''
      runHook preBuild
      make -j"$NIX_BUILD_CORES" libfdt/libfdt.a
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      install -D -m644 -t "$out/lib/" libfdt/libfdt.a
      install -D -m644 -t "$dev/include/fdt" libfdt/*.h
      runHook postInstall
    '';
  };

  dev = pkgs.lib.getDev self;
in
  self.overrideAttrs (prev: {
    passthru = {
      include_root = "${dev}/include";
      include      = "${dev}/include/fdt";
      lib          = "${self}/lib";
    };
  })

