{ nixpkgs ? <nixpkgs> , pkgs ? import nixpkgs {}, stdenv ? pkgs.stdenv } : 
let
  tools = stdenv.mkDerivation rec {
    name = "includeos-tools";
    src = pkgs.lib.cleanSource ./.;
    nativeBuildInputs = [
      pkgs.cmake      
    ];
    buildInputs = [
      pkgs.microsoft_gsl
    ];
  };
in
tools
  

