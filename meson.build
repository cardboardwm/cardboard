project(
  'cardboardwm',
  'c', 'cpp',
  version: '0.0.1',
  license: 'XXX',
  default_options: [
	'c_std=c17',
	'cpp_std=gnu++17',
	'warning_level=2',
	'werror=true',
	'default_library=static'
  ],
)

add_project_arguments(
  [
	'-DWLR_USE_UNSTABLE',
  ],
  language: 'cpp'
)

inc = include_directories('include')
wlroots = dependency('wlroots')
wayland_server = dependency('wayland-server')

subdir('protocols')
subdir('cardboard')
