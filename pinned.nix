import (
  builtins.fetchTarball {
    #
    # Pinned to nixpkgs-unstable, commit bcd44e2 from May 1st. 2024:
    # https://github.com/NixOS/nixpkgs/commit/bcd44e224fd68ce7d269b4f44d24c2220fd821e7
    #
    # Fetched from nixpkgs-unstable branch May 9th. 2024 (Ascension day)
    # Found the hash in /nix/var/nix/profiles/per-user/root/channels/nixpkgs/.git-revision
    #
    url = "https://github.com/nixos/nixpkgs/archive/bcd44e224fd68ce7d269b4f44d24c2220fd821e7.tar.gz";
    sha256 = "1dd8x811mkm0d89b0yy0cgv28g343bnid0xn2psd3sk1nkgx9g9j";

    # TODO: We want to pin to a nixpkgs release branch, but:
    #
    # This👇 "is not able to compile a simple test program" (clang via cmake)
    # url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/24.05-pre.tar.gz";
    # sha256 = "1cfbkahcfj1hgh4v5nfqwivg69zks8d72n11m5513i0phkqwqcgh";

    # This👇 "is not able to compile a simple test program" (clang via cmake)
    # url = "https://github.com/NixOS/nixpkgs/archive/refs/tags/23.11.tar.gz";
    # sha256 = "1ndiv385w1qyb3b18vw13991fzb9wg4cl21wglk89grsfsnra41k";
    #
  }
)
