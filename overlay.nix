final: prev: {
  pkgsIncludeOS = prev.pkgsStatic.lib.makeScope prev.pkgsStatic.newScope (self: {
    # self.callPackage will use this stdenv.
    stdenv = prev.pkgsStatic.llvmPackages_16.libcxxStdenv;

    # Deps
    musl-includeos = self.callPackage ./deps/musl/default.nix { };
    uzlib = self.callPackage ./deps/uzlib/default.nix { };
    botan2 = self.callPackage ./deps/botan/default.nix { };
    microsoft_gsl = self.callPackage ./deps/GSL/default.nix { };
    s2n-tls = self.callPackage ./deps/s2n/default.nix { };
    http-parser = self.callPackage ./deps/http-parser/default.nix { };

    # IncludeOS
    includeos = self.stdenv.mkDerivation rec {
      pname = "includeos";

      version = "dev";

      src = prev.pkgsStatic.lib.cleanSource ./.;

      # If you need to patch, this is the place
      postPatch = '''';

      nativeBuildInputs = [
        prev.cmake
        prev.nasm
      ];

      buildInputs = [

        # TODO:
        # including musl here makes the compiler pick up musl's libc headers
        # before libc++'s libc headers, which doesn't work. See e.g. <cstdint>
        # line 149;
        # error <cstdint> tried including <stdint.h> but didn't find libc++'s <stdint.h> header.
        # #ifndef _LIBCPP_STDINT_H
        #   error <cstdint> tried including <stdint.h> but didn't find libc++'s <stdint.h> header. \
        #   This usually means that your header search paths are not configured properly. \
        #   The header search paths should contain the C++ Standard Library headers before \
        #   any C Standard Library, and you are probably using compiler flags that make that \
        #   not be the case.
        # #endif
        #
        # With this commented out IncludeOS builds with Nix libc++, which is great,
        # but might bite us later because it then uses a libc we didn't patch.
        # Best case we case we can adapt our own musl to be identical, use nix static
        # musl for compilation, and includeos-musl only for linking.
        #
        # musl-includeos  👈 this has to come in after libc++ headers.
        self.botan2
        self.http-parser
        self.microsoft_gsl
        prev.pkgsStatic.openssl
        prev.pkgsStatic.rapidjson
        #self.s2n-tls          👈 This is postponed until we can fix the s2n build.
        self.uzlib
      ];

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
