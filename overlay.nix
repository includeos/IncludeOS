{
  withCcache, # Enable ccache. Requires correct permissions, see below.
  disableTargetWarning ? true, # TODO: see https://github.com/NixOS/nixpkgs/issues/395191
  smp,      # Enable multicore support (SMP)
} :
final: prev: {

  stdenvIncludeOS = prev.pkgsStatic.lib.makeScope prev.pkgsStatic.newScope (self: {
    llvmPkgs = prev.pkgsStatic.llvmPackages_20;
    stdenv = self.llvmPkgs.libcxxStdenv; # Use this as base stdenv

    # Import unpatched musl for building libcxx. Libcxx needs some linux headers to be passed through.
    musl-unpatched = self.callPackage ./deps/musl-unpatched/default.nix { linuxHeaders = prev.linuxHeaders; };

    # Import IncludeOS musl which will be built and linked with IncludeOS services
    musl-includeos = self.callPackage ./deps/musl/default.nix { };

    # Clang with unpatched musl for building libcxx
    clang_musl_unpatched_nolibcxx = self.llvmPkgs.clangNoLibcxx.override (old: {
      bintools = prev.pkgsStatic.bintools.override {
        # Disable hardening flags while we work on the build
        defaultHardeningFlags = [];
        libc = self.musl-unpatched;
      };
      libc = self.musl-unpatched;
    });

    # Libcxx which will be built with unpatched musl
    libcxx_musl_unpatched = self.llvmPkgs.libcxx.override (old: {
      stdenv = (prev.overrideCC self.llvmPkgs.libcxxStdenv self.clang_musl_unpatched_nolibcxx);
    });

    # Final stdenv, use libcxx w/unpatched musl + includeos musl as libc
    clang_musl_includeos_libcxx = self.llvmPkgs.libcxxClang.override (old: {
      bintools = prev.pkgsStatic.bintools.override {
        # Disable hardening flags while we work on the build
        defaultHardeningFlags = [];
        libc = self.musl-includeos;
      };
      libc = self.musl-includeos;
      libcxx = self.libcxx_musl_unpatched;
    });

    musl_includeos_stdenv_libcxx = (prev.overrideCC self.llvmPkgs.libcxxStdenv self.clang_musl_includeos_libcxx);

    includeos_stdenv = self.musl_includeos_stdenv_libcxx;

    libraries = {
      libc = self.musl-includeos;
      libcxx = {
        # There doesn't seem to be a single package containing both libc++ headers and libs.
        lib = "${self.libcxx_musl_unpatched}/lib";
        include = "${self.libcxx_musl_unpatched.dev}/include/c++/v1";
      };
      libunwind = self.llvmPkgs.libraries.libunwind;
      libgcc = self.llvmPkgs.compiler-rt;
    };
  });

  pkgsIncludeOS = prev.pkgsStatic.lib.makeScope prev.pkgsStatic.newScope (self:
    let
      ccacheNoticeHook = prev.writeTextFile {
        name = "ccache-notice-hook";
        destination = "/nix-support/setup-hook";
        text = ''
          echo "====="
          echo "ccache is enabled!"
          echo "If you run into any issues, try: --arg withCcache false"
          echo "It's recommended to run tests with ccache disabled to avoid cache incoherencies."
          echo "====="
        '';
      };

      suppressTargetWarningHook = prev.writeTextFile {
        name = "suppress-target-warning-hook";
        destination = "/nix-support/setup-hook";
        text = ''
          # see https://github.com/NixOS/nixpkgs/issues/395191
          # delete this hook and downstream references once resolved

          export NIX_CC_WRAPPER_SUPPRESS_TARGET_WARNING=1
        '';
      };
    in {
    # self.callPackage will use this stdenv.
    stdenv = final.stdenvIncludeOS.includeos_stdenv;
    inherit suppressTargetWarningHook;

    # Deps
    botan2 = self.callPackage ./deps/botan/default.nix { };
    libfmt = self.callPackage ./deps/libfmt/default.nix { };
    s2n-tls = self.callPackage ./deps/s2n/default.nix { };
    uzlib = self.callPackage ./deps/uzlib/default.nix { };

    vmbuild = self.callPackage ./vmbuild.nix { };

    ccacheWrapper = prev.ccacheWrapper.override {
        inherit (self.stdenv) cc;
        extraConfig = ''
          export CCACHE_DIR="/nix/var/cache/ccache"
          if [ ! -d "$CCACHE_DIR" ]; then
            echo "====="
            echo "Directory '$CCACHE_DIR' does not exist"
            echo "Please create it with:"
            echo "  sudo mkdir -m0770 '$CCACHE_DIR'"
            echo "  sudo chown root:nixbld '$CCACHE_DIR'"
            echo ""
            echo 'Alternatively, disable ccache with `--arg withCcache false`'
            echo "====="
            exit 1
          fi
          if [ ! -w "$CCACHE_DIR" ]; then
            echo "====="
            echo "Directory '$CCACHE_DIR' exists, but is not accessible for user $(whoami)"
            echo "Please verify its access permissions"
            echo 'Alternatively, disable ccache with `--arg withCcache false`'
            echo "====="
            exit 1
          fi

          export CCACHE_COMPRESS=1
          export CCACHE_UMASK=007
          export CCACHE_SLOPPINESS=random_seed
        '';
      };

    # IncludeOS
    includeos = self.stdenv.mkDerivation rec {
      enableParallelBuilding = true;
      pname = "includeos";

      version = "dev";

      # Convenient access to libc, libcxx etc
      passthru.libraries = final.stdenvIncludeOS.libraries;

      src = prev.pkgsStatic.lib.fileset.toSource {
          root = ./.;
          # Only include files needed by IncludeOS (not examples, docs etc)
          fileset = prev.pkgsStatic.lib.fileset.unions [
            ./src
            ./api
            ./cmake
            ./deps
            ./userspace
            ./lib
            ./CMakeLists.txt
          ];
      };

      # If you need to patch, this is the place
      postPatch = '''';

      nativeBuildInputs = [
        prev.buildPackages.cmake
        prev.buildPackages.nasm
      ] ++ prev.lib.optionals disableTargetWarning [suppressTargetWarningHook]
        ++ prev.lib.optionals withCcache [self.ccacheWrapper ccacheNoticeHook];

      buildInputs = [
        self.libfmt
        self.botan2
        prev.pkgsStatic.http-parser
        prev.pkgsStatic.openssl
        prev.pkgsStatic.rapidjson
        #self.s2n-tls          👈 This is postponed until we can fix the s2n build.
        self.uzlib
        self.vmbuild
      ];

      postInstall = ''
        echo Copying vmbuild binaries to tools/vmbuild
        mkdir -p "$out/tools/vmbuild"
        cp -v ${self.vmbuild}/bin/* "$out/tools/vmbuild"
        cp -r -v ${final.stdenvIncludeOS.libraries.libc} $out/libc
        mkdir $out/libcxx
        cp -r -v ${final.stdenvIncludeOS.libraries.libcxx.lib} $out/libcxx/lib
        cp -r -v ${final.stdenvIncludeOS.libraries.libcxx.include} $out/libcxx/include
        cp -r -v ${final.stdenvIncludeOS.libraries.libunwind} $out/libunwind
        cp -r -v ${final.stdenvIncludeOS.libraries.libgcc} $out/libgcc
        cp -r -v ${final.pkgsStatic.http-parser} $out/http-parser
      '';

      archFlags = if self.stdenv.targetPlatform.system == "i686-linux" then
        [
          "-DARCH=i686"
          "-DPLATFORM=nano" # we currently only support nano platform on i686
        ]
      else
        [ "-DARCH=x86_64"];

      smpFlags = if smp then [ "-DSMP=ON" ] else [];

      cmakeFlags = archFlags ++ smpFlags;

      # Add some pasthroughs, for easily building the dependencies (for debugging):
      # $ nix-build -A NAME

      passthru.vmrunner = prev.callPackage (builtins.fetchGit {
          url = "https://github.com/includeos/vmrunner";
        }) {};
      passthru.chainloader = import ./chainloader.nix { inherit withCcache; };
      passthru.lest = self.callPackage ./deps/lest {};
      passthru.pkgsStatic = prev.pkgsStatic; # this is for convenience for other packages that depend on includeos
      passthru.pkgs = prev.pkgs; # this is for convenience for other packages that depend on includeos

      passthru = {
        inherit (self) uzlib;
        inherit (self) botan2;
        inherit (self) libfmt;
        #inherit (self) s2n-tls;
        inherit (self) cmake;
        inherit (self) vmbuild;
      };

      meta = {
        description = "Run your application with zero overhead";
        homepage = "https://www.includeos.org/";
        license = prev.pkgsStatic.lib.licenses.asl20;
      };
    };
  });
}
