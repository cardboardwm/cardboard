# SPDX-License-Identifier: GPL-3.0-only
project(
  'cardboardwm',
  'c', 'cpp',
  version: '0.0.1',
  license: 'XXX',
  default_options: [
    'c_std=c17',
    'cpp_std=c++2a',
    'warning_level=2',
    'werror=true',
    'default_library=static'
  ],
)

add_project_arguments(
  [
    '-DWLR_USE_UNSTABLE',
    '-Wno-missing-field-initializers',
  ],
  language: 'cpp'
)

# workaround for wlroots 0.10.1 on gcc 10
add_global_arguments(
  ['-fcommon'], language: 'c'
)

datadir = get_option('datadir')

install_data('cardboard.desktop', install_dir: join_paths(datadir, 'wayland-sessions'))

subdir('protocols')
subdir('libcardboard')
subdir('cardboard')
subdir('cutter')
