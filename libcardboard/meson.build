inc = include_directories('include')

cereal = dependency('cereal')

sources = files(
    'src/command_protocol.cpp',
    'src/ipc.cpp',
)

install_headers('include/command_protocol.h')

libcardboard = library(
    'command_protocol',
    sources,
    include_directories: inc,
    install: true,
    dependencies: [cereal]
)
