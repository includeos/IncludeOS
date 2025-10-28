{
  pkgs,
  stdenv
}:
stdenv.mkDerivation rec {
  pname = "lest";
  version = "1.37.0";

  meta = {
    description = "A tiny C++11 test framework – lest errors escape testing.";
    homepage = "https://github.com/martinmoene/lest";
    license = pkgs.lib.licenses.boost;
  };

  src = fetchGit {
    url = "https://github.com/martinmoene/lest.git";
    ref = "refs/tags/v${version}";
    rev = "fe1996f438ab8d1611c0324a156f9130ed971e9f";
  };

  cmakeBuildType = "Debug";

  postBuild = ''
      mkdir -p "$out/include"
      cp -r include "$out/"
    '';

}
