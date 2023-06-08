# UNMAINTAINED. Read the explanation bellow

If you are curious to see what this project was about, you can check out [this cool blog post written by Daniel Aleksandersen][blog-post].

If the scrollable tiling concept intrigues you, you can either:
* Use [PaperWM on GNOME](https://github.com/paperwm/PaperWM), or
* Check out [Karousel](https://github.com/peterfajdiga/karousel), a KWin tiling script.

We (Alex and Tudor) had great fun while developing Cardboard, allowing us to learn more about Wayland and C++ development.
Cardboard was made as a project for the [National Digital Innovation and Creativity Olympiad - InfoEducație][infoedu],
a government-sponsored software development contest, camp, and hackathon* for Romanian high school students.
We figured it would be great to make a project that is based on new Linux technologies while also helping the
Wayland community with a compositor implementing a novel tiling algorithm. We have since started university and working
on the side, which doesn't allow us to maintain the messy codebase we left behind.
Wlroots has changed in the meantime too, requiring us to keep up with its changes to keep our compositor functional.
None of us use this software day to day, so naturally this task didn't receive much priority, if at all.

Still, we would like to keep this project public, for other to learn from it.
For a more up to date version, you can try [Julien Langlois][fork]' fork. We are thankful for his effort to touch up our codebase.

If you want to build your own compositor, we have [a couple pointers][pointers]:

* Make use of the new [Wlroots Scene Graph API][wlr_scene] as much as possible. Such a luxury wasn't yet upstreamed
	when we were working on Cardboard.
* As always, a very minimal implementation of many Wlroots goodies (including the scene graph API) is
	[cage][cage]. Read and study the code.
* Read [this blog post written by Tudor][the-wayland-experience]. It was written after coding Cardboard. As such,
	it doesn't take into consideration the now-released scene graph API.
* Have fun!

*: For our hackathon submission, see [The X Files ISA][x-files-isa].

[infoedu]: https://infoeducatie.ro/home
[x-files-isa]: https://gitlab.com/cardboardwm/x-files-isa
[fork]: https://gitlab.com/langlois-jf/cardboard
[pointers]: https://xkcd.com/138/
[wlr_scene]: https://wayland.emersion.fr/wlroots/wlr/types/wlr_scene.h.html
[cage]: https://github.com/cage-kiosk/cage
[the-wayland-experience]: https://tudorr.ro/blog/technical/2021/01/26/the-wayland-experience/
[blog-post]: https://www.ctrl.blog/entry/cardboardwm.html

# Cardboard

Cardboard is a unique, scrollable tiling Wayland compositor designed with
laptops in mind. Based on [wlroots](https://github.com/swaywm/wlroots).

Cardboard is experimental software, and no warranty should be derived from it.

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

### Building man pages
In order to build the man pages, along the binary, the meson `man` variable
needs to be set to `true`:

```sh 
$ meson build -Dman=true
```

## Configuration

Cardboard tries to run `~/.config/cardboard/cardboardrc` on startup. You can use
to run commands and set keybindings:

``` sh
#!/bin/sh

alias cutter=$HOME/src/cardboard/build/cutter/cutter

mod=alt

cutter config mouse_mod $mod

cutter bind $mod+x quit
cutter bind $mod+return exec sakura
cutter bind $mod+left focus left
cutter bind $mod+right focus right
cutter bind $mod+shift+left move -10 0
cutter bind $mod+shift+right move 10 0
cutter bind $mod+shift+up move 0 -10
cutter bind $mod+shift+down move 0 10
cutter bind $mod+w close
for i in $(seq 1 6); do
	cutter bind $mod+$i workspace switch $(( i - 1 ))
	cutter bind $mod+ctrl+$i workspace move $(( i - 1 ))
done

cutter bind $mod+shift+space toggle_floating

cutter exec toolbox run swaybg -i ~/wallpapers/autobahn.png
cutter exec toolbox run waybar
```

As you can see, to run a program, you need to preffix it with `cutter exec`.

Have fun!
