# Cardboard

Cardboard is a unique, scrollable tiling Wayland compositor designed with
laptops in mind. Based on [wlroots](https://github.com/swaywm/wlroots).

It is currently a Work In Progress, not suitable for day to day use. We will
reach this target in a short time. [To do list](https://gitlab.com/cardboardwm/cardboard/-/wikis/todo).

## Scrollable tiling?

The idea of scrollable tiling comes from
[PaperWM](https://github.com/paperwm/PaperWM). Instead of tiling by filling the
screen with windows placed in a tree-like container structure, in scrollable
tiling, each workspace is a series of vertically-maximized windows placed one
after another, overflowing the screen. The monitor becomes a sliding window over
this sequence of windows, allowing for easier window placement. The main idea
is that you are not going to look at more than about three windows at the same
time. While in other tiling compositors you can put the lesser-used windows in
different workspaces, you will have to remember their position in your head and
deal with juggling through the workspaces. With scrollable tiling, unused
windows are just placed off-screen, and you can always scroll back to see them.
Cardboard also provides workspaces, each with its own distinct scrollable plane.
Each output has its own active workspace.

## Cutter

Cardboard has a companion program named Cutter. Cutter is a tool for
communicating actions and getting information to and from cardboard, similar to
`i3msg` on i3, `swaymsg` on Sway and `bspc` on bspwm. It is also the program for
configuring Cardboard. Config files are basic executable files that should call
Cutter to set config values. Cutter itself is based on `libcardboard`, a library
to easily communicate with Cardboard.

## Building the code

Cardboard requires Meson and Ninja to build the code.

Libraries:

* wayland-server
* xkbcommon
* wlroots

The build script won't use the system wlroots. Instead, it will try to compile
its own, so make sure you have its dependencies installed. Cardboard supports
Xwayland, so if you enable it (it's enabled by default, disable with `meson
-Dxwayland=disabled <build_dir>`), you
also need what wlroots needs to get Xwayland working.

You should also have a C++ compiler that compiles in `-std=c++2a` mode. Currently,
compilation works with `-std=c++17` too, but this might break soon. You need to
edit the root `meson.build` file to compile in C++ 17 mode.

Get the code, build and run:

``` sh
$ git clone https://gitlab.com/cardboardwm/cardboard
$ cd cardboard
$ meson build
$ ninja -C build
$ ./build/cardboard/cardboard # to run the thing
```

Cardboard tries to run `~/.config/cardboard/cardboardrc` on startup. You can use
to run commands and set keybindings:

``` sh
#!/bin/sh

alias cutter=$HOME/src/cardboard/build/cutter/cutter

cutter bind alt+x quit
cutter bind alt+return exec xfce4-terminal
cutter bind alt+left focus left
cutter bind alt+right focus right
cutter exec swaybg -i ~/wallpapers/coolwall.png
cutter exec waybar
```

As you can see, to run a program, you need to preffix it with `cutter exec`.

Have fun!
