{
  stdenv
}:
let
  # Add needed $out/include/http-parser directory to match IncludeOS' use of
  # "#include <http-parser/http_parser.h>".
  # TODO: Upstream doesn't use that subdir though, so better fix IncludeOS
  # sources.
  #
  # Uses a more recent version of nixpkgs to get support for static builds  # TODO: verify if still obsolete
  nixpkgsHttpfix = builtins.fetchTarball {
      url = "https://github.com/NixOS/nixpkgs/archive/33f464b661f939689aa56af6b6e27b504c5afb93.tar.gz";
      sha256 = "15bdlccjg14qa7lwkcc7pikvi386ig108ca62hbxfas5wyw1fr62";
  };
  pkgsHttpfix = import nixpkgsHttpfix { crossSystem = { config = stdenv.targetPlatform.config; }; };
  pkgs = pkgsHttpfix.pkgsStatic;

  self = pkgs.http-parser;

  dev = pkgs.lib.getDev self;
  lib = pkgs.lib.getLib self;
in
  self.overrideAttrs (oldAttrs: {
    inherit stdenv;
    postInstall = (oldAttrs.postInstall or "") + ''
      mkdir "$out/include/http-parser"
      ln -sr "$out/include/http_parser.h" "$out/include/http-parser"
    '';
    passthru = (oldAttrs.passthru or {}) // {
      include_root = "${dev}/include";
      include = "${dev}/include";  # TODO: consider subdir?
      lib = "${self}/lib";
    };
  })
