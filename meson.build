project(
  'cardboardwm',
  'c', 'cpp',
  version: '0.0.1',
  license: 'XXX',
  default_options: [
    'c_std=c17',
    'cpp_std=gnu++2a',
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

include_dir = include_directories('include')

subdir('protocols')
subdir('libcommand_protocol')
subdir('cardboard')
subdir('cutter')
