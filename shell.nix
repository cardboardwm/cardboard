let
  pkgs = import <nixpkgs> {};
in
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    meson
    ninja
    pkg-config
    gcc10
  ] ++ pkgs.wlroots.nativeBuildInputs;

  buildInputs = with pkgs; [
    wayland
    libffi
  ] ++ pkgs.wlroots.buildInputs;
}
