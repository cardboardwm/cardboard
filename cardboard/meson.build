wayland_server = dependency('wayland-server')
xkbcommon = dependency('xkbcommon')
xcb = dependency('xcb', required: get_option('xwayland'))

wlroots_version = '>=0.10.0'
wlroots_proj = subproject(
  'wlroots',
  default_options: ['examples=false'],
  required: true,
  version: wlroots_version,
)

wlroots = wlroots_proj.get_variable('wlroots')
wlroots_conf = wlroots_proj.get_variable('conf_data')
wlroots_has_xwayland = wlroots_conf.get('WLR_HAS_XWAYLAND') == 1

if get_option('xwayland').enabled() and not wlroots_has_xwayland
    error('Cannot enable Xwayland support in cardboard: wlroots has been built without Xwayland support')
endif
have_xwayland = xcb.found() and wlroots_has_xwayland

expected_proj = subproject('expected', required: true)
expected = expected_proj.get_variable('expected_dep')

conf_data = configuration_data()
conf_data.set10('HAVE_XWAYLAND', have_xwayland)

configure_file(output: 'BuildConfig.h', configuration: conf_data)

cardboard_deps = [
  expected,
  wayland_server,
  wlroots,
  xkbcommon,
  server_protos,
]

cardboard_sources = files(
  'Cursor.cpp',
  'IPC.cpp',
  'Keyboard.cpp',
  'Layers.cpp',
  'Output.cpp',
  'OutputManager.cpp',
  'Seat.cpp',
  'Server.cpp',
  'Spawn.cpp',
  'View.cpp',
  'Workspace.cpp',
  'XDGView.cpp',
  'ViewOperations.cpp',
  'main.cpp',
  'commands/dispatch_command.cpp'
)

if have_xwayland
    cardboard_deps += xcb
    cardboard_sources += 'Xwayland.cpp'
endif

subdir('wlr_cpp_fixes')

executable(
  'cardboard',
  cardboard_sources,
  include_directories: [wlr_cpp_fixes_inc, libcardboard_inc],
  dependencies: cardboard_deps,
  link_with: libcardboard,
  install: true
)
