let
  pkgs = import <nixpkgs> {};
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    meson
    ninja
    pkg-config
    wayland
    libffi
  ] ++ pkgs.wlroots.nativeBuildInputs;

  buildInputs = pkgs.wlroots.buildInputs;
}
