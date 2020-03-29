inc = include_directories('include')

capnp = dependency('capnp')
kj = dependency('kj')

sources = files(
    'src/command.capnp.c++',
    'src/command_protocol.cpp'
)

install_headers('include/command_protocol.h')

command_protocol = library(
    'command_protocol',
    sources,
    include_directories: inc,
    install: true,
    dependencies: [capnp, kj]
)
