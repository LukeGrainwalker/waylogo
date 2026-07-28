
help2man = find_program('help2man')

#meson.project_build_root() / 'waylogo'

man = custom_target('generate man file',
  depends: exec,
  input: exec, #meson.project_build_root() / 'waylogo',
  output: 'waylogo.1',
  command:[help2man, '-o', '@OUTPUT@', '--help-option=-help', 
  '--version-option=-version', '@BUILD_ROOT@/@INPUT@'],
  build_by_default:true,
  install: true,
  install_dir:get_option('prefix') / get_option('mandir') / 'man1',
)

