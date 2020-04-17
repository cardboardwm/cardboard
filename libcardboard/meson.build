inc = include_directories('include')

cereal = dependency('cereal')

expected_proj = subproject('expected', required: true)
expected = expected_proj.get_variable('expected_dep')

sources = files(
    'src/command_protocol.cpp',
    'src/ipc.cpp',
)

libcardboard = library(
    'command_protocol',
    sources,
    include_directories: inc,
    install: true,
    dependencies: [cereal, expected]
)
