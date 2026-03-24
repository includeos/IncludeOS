{
  description = "IncludeOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/25.05";
    vmrunner.url = "github:includeos/vmrunner";
  };

  outputs = { self, nixpkgs, vmrunner }:
    let
      system = "x86_64-linux";

      mkIncludeos = { withCcache ? false, smp ? false }:
        let
          overlays = [
            (import ./overlay.nix {
              inherit withCcache smp;
              disableTargetWarning = true;
            })
          ];
          pkgs            = import nixpkgs { config = {}; inherit overlays system; };
          pkgsIncludeOS   = pkgs.pkgsIncludeOS;
          stdenvIncludeOS = pkgs.stdenvIncludeOS;
          includeos       = pkgsIncludeOS.includeos;

          chainloader =
            let
              chainloaderPkgs = import nixpkgs {
                inherit system;
                config = {};
                overlays = [
                  (import ./overlay.nix { inherit withCcache; smp = false; disableTargetWarning = true; })
                ];
                crossSystem = { config = "i686-unknown-linux-musl"; };
              };
            in import ./chainloader.nix {
                inherit nixpkgs withCcache;
                pkgs = chainloaderPkgs;
              };
        in {
          inherit stdenvIncludeOS;  # modified stdenv scope used by includeos (llvm, musl, libcxx)
          inherit pkgsIncludeOS;    # the musl/clang package scope used by IncludeOS
          inherit pkgs;             # nixpkgs with the IncludeOS overlay applied
          inherit includeos;        # the IncludeOS derivation to add to buildInputs
          inherit chainloader;      # 32-bit boot stub that hotswaps into the 64-bit unikernel
        };

      default = mkIncludeos {};
      mkUnikernel = import ./mkUnikernel.nix { inherit system default vmrunner; };
    in {
      packages.${system} = {
        default     = default.includeos;
        chainloader = default.chainloader;
        example     = mkUnikernel { unikernel = ./example; };
      };

      devShells.${system}.default = import ./develop.nix { includeos = default.includeos; };

      lib.${system} = { inherit mkIncludeos mkUnikernel; };

      apps.${system} = {
        default = self.apps.${system}.boot-unikernel;

        boot = {
          type = "app";
          program = "${vmrunner.lib.${system}.mkBoot default.chainloader}/bin/boot";
        };

        boot-unikernel = {
          type = "app";
          program = "${default.pkgs.writeShellScript "boot-unikernel" ''
            set -e
            dir="''${1:-./result}"
            shift || true
            exec ${vmrunner.lib.${system}.mkBoot default.chainloader}/bin/boot \
              -j $dir/vm.json \
              $dir/*.elf.bin \
              "$@"
          ''}";
        };

        example = {
          type = "app";
          program = "${default.pkgs.writeShellScript "run-example" ''
            set -e
            built=$(nix build path:${self.outPath}#example --no-link --print-out-paths)
            exec ${self.apps.${system}.boot-unikernel.program} $built "$@"
          ''}";
        };
      };
    };
}
