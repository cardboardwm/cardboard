inc = include_directories('include')

sources = files(
    'src/command.capnp.c++',
    'src/command_protocol.cpp'
)

install_headers('include/command_protocol.h')

library(
    'command_protocol',
    sources,
    include_directories: inc,
    install: true
)