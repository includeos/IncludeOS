{stdenv}:
let
  nix_20_09 = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/38eaa62f28384bc5f6c394e2a99bd6a4913fc71f.tar.gz";
    sha256 = "1pvbhvy6m5zmhhifk66ll07fnwvwnl9rrif03i4yc34s4f48m7ld";
  }) { inherit stdenv; };
in
nix_20_09.microsoft_gsl
