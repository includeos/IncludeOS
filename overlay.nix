final: prev: {
  pkgsIncludeOS = prev.pkgsStatic.lib.makeScope prev.pkgsStatic.newScope (self: rec {
    # self.callPackage will use this stdenv.
    llvmPkgs = prev.pkgsStatic.llvmPackages_16;
    stdenv = llvmPkgs.libcxxStdenv;

    # Deps
    uzlib = self.callPackage ./deps/uzlib/default.nix { };
    botan2 = self.callPackage ./deps/botan/default.nix { };
    microsoft_gsl = self.callPackage ./deps/GSL/default.nix { };
    s2n-tls = self.callPackage ./deps/s2n/default.nix { };
    http-parser = self.callPackage ./deps/http-parser/default.nix { };

    # Create new stdenv with musl to build includeos. This could also be used to build the dependencies,
    # but currently (June 5th, 2024) this fails on libuv tests (and is a very large build).
    musl-includeos = self.callPackage ./deps/musl/default.nix { };
    clang_musl = llvmPkgs.libcxxClang.override (old: {
      bintools = llvmPkgs.bintools.override {
        libc = musl-includeos;
      };
      libc = musl-includeos;
    });
    musl_stdenv = (prev.overrideCC stdenv clang_musl);

    # IncludeOS
    includeos = musl_stdenv.mkDerivation rec {
      enableParallelBuilding = true;
      hardeningDisable = [ "pie" ]; # use "all" to disable all hardening options
      pname = "includeos";

      version = "dev";

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
        prev.cmake
        prev.nasm
      ];

      buildInputs = [

        self.botan2
        self.http-parser
        self.microsoft_gsl
        prev.pkgsStatic.openssl
        prev.pkgsStatic.rapidjson
        prev.linuxHeaders # needed for linux/limits.h
        #self.s2n-tls          👈 This is postponed until we can fix the s2n build.
        self.uzlib
      ];

      archFlags = if self.stdenv.targetPlatform.system == "i686-linux" then
        [
          "-DARCH=i686"
          "-DPLATFORM=nano" # we currently only support nano platform on i686
        ]
      else
        [ "-DARCH=x86_64"];

      cmakeFlags = archFlags;

      # Add some pasthroughs, for easily building the depdencies (for debugging):
      # $ nix-build -A NAME
      passthru = {
        inherit (self) uzlib;
        inherit (self) http-parser;
        inherit (self) botan2;
        #inherit (self) s2n-tls;
        inherit (self) musl-includeos;
        inherit (self) cmake;
      };

      meta = {
        description = "Run your application with zero overhead";
        homepage = "https://www.includeos.org/";
        license = prev.pkgsStatic.lib.licenses.asl20;
      };
    };
  });
}
