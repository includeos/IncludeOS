{ pkgs }:
let
  targetWarningHook = pkgs.writeTextFile {
    name = "suppress-target-warning-hook";
    destination = "/nix-support/setup-hook";
    text = ''
          # see https://github.com/NixOS/nixpkgs/issues/395191
          # delete this hook and downstream references once resolved

          export NIX_CC_WRAPPER_SUPPRESS_TARGET_WARNING=1
    '';
  };
in
{
  inherit targetWarningHook;
}
