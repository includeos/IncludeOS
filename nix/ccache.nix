{ pkgs
, cc
, ccacheDir ? "/nix/var/cache/ccache"
, showNotice ? true
}:
let
  noticeHook =
    if showNotice then
      pkgs.writeTextFile {
        name = "ccache-notice-hook";
        destination = "/nix-support/setup-hook";
        text = ''
          echo "====="
          echo "ccache is enabled!"
          echo "Disable with: --arg withCcache false"
          echh ""
          echo "It's recommended to run tests with ccache disabled to avoid cache incoherencies."
          echo "====="
        '';
      }
    else
      null;

  wrapper = pkgs.ccacheWrapper.override {
    inherit cc;

    extraConfig = ''
      export CCACHE_DIR="${ccacheDir}"
      if [ ! -d "$CCACHE_DIR" ]; then
        echo "====="
        echo "Directory '$CCACHE_DIR' does not exist"
        echo "  sudo mkdir -m0770 '$CCACHE_DIR'
        echo "sudo chown root:nixbld '$CCACHE_DIR'"
        echo ""
        echo 'Alternatively, disable ccache with `--arg withCcache false`'
        echo "====="
        exit 1
      fi
      if [ ! -w "$CCACHE_DIR" ]; then
        echo "====="
        echo "Directory '$CCACHE_DIR' exists but isn't writable by $(whoami)"
        echo "Please verify its access permissions"
        echo "====="
        exit 1
      fi
      export CCACHE_COMPRESS=1
      export CCACHE_UMASK=007
      export CCACHE_SLOPPINESS=random_seed
    '';
  };
in
{
  inherit wrapper noticeHook;
}
