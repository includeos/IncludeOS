# This build is dynamic
{
  pkgs,
  stdenv
}:
let
  self = stdenv.mkDerivation rec {
    pname = "s2n-tls";
    # ./conanfile.py lists 0.8, but there are not tags in the repo with version < 0.9.0
    version = "0.9.0";

    src = pkgs.fetchzip {
      url = "https://github.com/aws/s2n-tls/archive/v${version}.tar.gz";
      sha256 = "18qjqc2jrpiwdpzqxl6hl1cq0nfmqk8qas0ijpwr0g606av0aqm9";  # v0.9.0
      # hash = "sha256-aJRw1a/XJivNZS3NkZ4U6nC12+wY/aoNv33mbAzNl0k=";  # v1.5.27
    };

    patches = [ ./fix-strict-prototypes.patch ];

    buildInputs = [
      pkgs.pkgsStatic.openssl
    ];

    # ld: cannot find -lgcc_eh: No such file or directory
    # ld: have you installed the static version of the gcc_eh library ?
    buildPhase = ''
      runHook preBuild

      make bin

      runHook postBuild
    '';

    # Upstream Makefile has no install target
    # FIXME: looks like it does now: https://github.com/aws/s2n-tls/blame/73720795dbc37d295592f427e8c225cfafef39a0/Makefile#L106
    installPhase = ''
      runHook preInstall

      mkdir -p "$out/include"
      cp api/s2n.h "$out/include"

      mkdir -p "$out/lib"
      cp lib/libs2n.a lib/libs2n.so "$out/lib"

      runHook postInstall
    '';

    meta = {
      description = "An implementation of the TLS/SSL protocols";
      homepage = "https://github.com/aws/s2n-tls";
      license = pkgs.lib.licenses.asl20;
    };
  };

  dev = pkgs.lib.getDev self;
  lib = pkgs.lib.getLib self;
in
  self.overrideAttrs (oldAttrs: {
    # TODO: verify the {include, lib} paths. commented are Gentoo artifacts
    passthru = (prev.passthru or {}) // {
      include_root = "${dev}/include"; # /usr/include/s2n.h
      include = "${dev}/include/s2n";  # /usr/include/s2n/unstable/*.h
      lib = "${self}/lib";  # /usr/lib64 on Gentoo...
    };
  })
