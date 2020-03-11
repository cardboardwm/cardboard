wayland_server = dependency('wayland-server')
xkbcommon = dependency('xkbcommon')

wlroots_version = '>=0.10.0'
wlroots_proj = subproject(
  'wlroots',
  default_options: ['examples=false'],
  required: true,
  version: wlroots_version,
)

wlroots = wlroots_proj.get_variable('wlroots')

cardboard_deps = [
  wayland_server,
  wlroots,
  xkbcommon,
  server_protos,
]

cardboard_sources = files(
  'EventListeners.cpp',
  'IPC.cpp',
  'Keyboard.cpp',
  'Output.cpp',
  'Server.cpp',
  'Tiling.cpp',
  'View.cpp',
  'main.cpp',
)

executable(
  'cardboard',
  cardboard_sources,
  include_directories: [include_dir],
  dependencies: cardboard_deps,
)
