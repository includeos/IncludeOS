{
  pkgs,
  stdenv
}:
stdenv.mkDerivation rec {
  pname = "lest";
  version = "1.35.2";

  meta = {
    description = "A tiny C++11 test framework – lest errors escape testing.";
    homepage = "https://github.com/martinmoene/lest";
    license = pkgs.lib.licenses.boost;
  };

  src = fetchGit {
    url = "https://github.com/martinmoene/lest.git";
    rev = "1eda2f7c33941617fc368ce764b5fbeffccb59bc";
  };

  cmakeBuildType = "Debug";

  postBuild = ''
      mkdir -p "$out/include"
      cp -r include "$out/"
    '';

}
