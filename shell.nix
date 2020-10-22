# SPDX-License-Identifier: GPL-3.0-only
let
  sources = import ./nix/sources.nix;
  pkgs = import sources.nixpkgs {};
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
