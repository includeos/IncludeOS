# This build is dynamic
{
  pkgs,
  stdenv
}:
stdenv.mkDerivation rec {
  pname = "s2n-tls";
  # ./conanfile.py lists 0.8, but there are not tags in the repo with version < 0.9.0
  version = "0.9.0";

  src = pkgs.fetchzip {
    url = "https://github.com/aws/s2n-tls/archive/v${version}.tar.gz";
    sha256 = "18qjqc2jrpiwdpzqxl6hl1cq0nfmqk8qas0ijpwr0g606av0aqm9";
  };

  buildInputs = [
    pkgs.pkgsStatic.openssl
  ];

  # the default 'all' target depends on tests which are broken (see below)
  buildPhase = ''
    runHook preBuild

    make bin

    runHook postBuild
  '';

  # TODO: tests fail:
  # make -C unit
  # make[2]: Entering directory '/build/source/tests/unit'
  # Running s2n_3des_test.c                                    ... FAILED test 1
  # !((conn = s2n_connection_new(S2N_SERVER)) == (((void *)0))) is not true  (s2n_3des_test.c line 44)
  # Error Message: 'error calling mlock (Did you run prlimit?)'
  #  Debug String: 'Error encountered in s2n_mem.c line 103'
  # make[2]: *** [Makefile:44: s2n_3des_test] Error 1
  doCheck = false;

  # Upstream Makefile has no install target
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
}
