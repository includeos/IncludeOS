{
  description = "IncludeOS";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/25.05";
    # vmrunner.url = "github:includeos/vmrunner";
    vmrunner.url = "github:mazunki/vmrunner";
  };

  outputs = { self, nixpkgs, vmrunner }:
    let
      system = "x86_64-linux";

      mkIncludeos = { withCcache ? false, smp ? false, debug ? false }:
        let
          overlays = [
            (import ./overlay.nix {
              inherit withCcache smp debug;
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

      default      = mkIncludeos {};
      defaultDebug = mkIncludeos { debug = true; };
      mkUnikernel  = import ./mkUnikernel.nix { inherit system default defaultDebug vmrunner; };
    in {
      packages.${system} = {
        default     = default.includeos // { debug = defaultDebug.includeos; };
        chainloader = default.chainloader;
        example     = mkUnikernel { unikernel = ./example; };
      };

      devShells.${system}.default = import ./develop.nix { includeos = default.includeos; };

      lib.${system} = {
        inherit mkIncludeos mkUnikernel;

        mkChainloader = { mem }:
          vmrunner.lib.${system}.mkBoot default.chainloader { inherit mem; };

        boot = { kernel ? "service", mem, kvm ? false, debug ? false }:
          let
            chainloaders = self.lib.${system}.mkChainloader { inherit mem; };
            chainloader = if kvm then chainloaders.kvm
                         else if debug then chainloaders.debug
                         else chainloaders.default;
          in {
            type = "app";
            program = "${default.pkgs.writeShellScript "boot-unikernel" ''
              set -e
              dir="''${1:-./result}"
              ${default.pkgs.lib.optionalString debug ''echo "boot: $dir/${kernel}" >&2''}
              exec ${chainloader}/bin/boot "$dir/${kernel}" "$@"
            ''}";
          };

        mkBoot1 = { src, kernel ? "service", kvm ? false, mem ? "16G", debug ? false }:
          let
            bootApp = self.lib.${system}.boot { inherit kernel kvm debug mem; };
          in {
            type = "app";
            program = "${default.pkgs.writeShellScript "run-flake-package" ''
              set -e
              exec ${bootApp.program} ${src} "$@"
            ''}";
          };

        mkBoot = args:
        let
          debugSrc = args.debugSrc or args.src;
          base     = builtins.removeAttrs args [ "debugSrc" ];
          tcg   = self.lib.${system}.mkBoot1 (base // { kvm = false; debug = false; });
          kvm   = self.lib.${system}.mkBoot1 (base // { kvm = true;  debug = false; });
          debug = self.lib.${system}.mkBoot1 (base // { src = debugSrc; kvm = false; debug = true; });
        in
          tcg // { inherit tcg kvm debug; };
      };

      apps.${system} = {
        default = self.apps.${system}.boot-unikernel;

        boot-unikernel = self.lib.${system}.boot { mem = "128m"; };

        example = self.lib.${system}.mkBoot { src = self.packages.${system}.example; kernel = "hello_includeos.elf.bin"; };
      };
    };
}
