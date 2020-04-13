inc = include_directories('include')

capnp = dependency('capnp')
kj = dependency('kj')

sources = files(
    'src/command.capnp.c++',
    'src/command_protocol.cpp',
    'src/ipc.cpp',
)

install_headers('include/command_protocol.h')

libcardboard = library(
    'command_protocol',
    sources,
    include_directories: inc,
    install: true,
    dependencies: [capnp, kj]
)
