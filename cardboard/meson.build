wayland_server = dependency('wayland-server')
wlroots = dependency('wlroots')
xkbcommon = dependency('xkbcommon')

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
