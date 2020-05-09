expected_proj = subproject('expected', required: true)
expected = expected_proj.get_variable('expected_dep')

executable(
    'cutter',
    files('main.cpp'),
    include_directories: [libcardboard_inc],
    link_with: libcardboard,
    dependencies: [expected],
    install: true
)
