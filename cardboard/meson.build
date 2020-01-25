wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots')
xkbcommon = dependency('xkbcommon')

wlroots_version = '>=0.10.0'
wlroots_proj = subproject(
  'wlroots',
  default_options: ['examples=false'],
  required: false,
  version: wlroots_version,
)

if wlroots_proj.found()
  wlroots = wlroots_proj.get_variable('wlroots')
  # TODO: xwayland checks
else
  wlroots = dependency('wlroots', version: wlroots_version)
endif

cardboard_deps = [
  wayland_server,
  wlroots,
  xkbcommon,
  server_protos,
]

cardboard_sources = files(
  'Server.cpp',
  'EventListeners.cpp',
  'Output.cpp',
  'Keyboard.cpp',
  'View.cpp',
  'main.cpp',
)

executable(
  'cardboard',
  cardboard_sources,
  include_directories: [include_directories('../include'), include_directories('.')],
  dependencies: cardboard_deps,
)
