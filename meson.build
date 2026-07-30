project('waylogo', 'c', version:'0.10', meson_version: '>=1.6.0')

wayland_scanner_dep = dependency('wayland-scanner',
  fallback: 'wayland',
  default_options: ['tests=false','documentation=false'],
  native: true,
)

wayland_scanner = find_program(
  wayland_scanner_dep.get_variable('wayland_scanner'),
  native: true,
)

wldep = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols',
  fallback: 'wayland-protocols',
  default_options: ['tests=false'],
)
wl_proto_dir = wayland_protos.get_variable('pkgdatadir')

wl_cursor_dep = dependency('wayland-cursor')

xdg_shell_path = wl_proto_dir / 'stable/xdg-shell/xdg-shell.xml'
xdg_shell_code = custom_target('xdg_shell_c',
  input: xdg_shell_path,
  output: '@BASENAME@-protocol.c',
  command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
)

xdg_shell_header = custom_target('xdg_shell_h',
  input: xdg_shell_path,
  output: '@BASENAME@-protocol.h',
  command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
)

plutovg_dep = dependency('plutovg', required: false)

if not plutovg_dep.found()
  plutovg_proj = subproject('plutovg', default_options: ['default_library=static'])
  plutovg_dep = plutovg_proj.get_variable('plutovg_dep')
endif

plutosvg_dep = dependency('plutosvg', required: false)

if not plutosvg_dep.found()
  plutosvg_proj = subproject('plutosvg', default_options: ['default_library=static'])
  plutosvg_dep = plutosvg_proj.get_variable('plutosvg_dep')
endif

conf_data = configuration_data()

conf_data.set('name', meson.project_name())
conf_data.set('version', meson.project_version())
conf_data.set('build_dir', meson.project_build_root())
conf_data.set('data_dir', get_option('prefix') / get_option('datadir') / meson.project_name())

configure_file(
  input: 'config.h.in',
  output: 'config.h',
  configuration: conf_data
)

curl = find_program('curl', required:false)
if curl.found()
  wayland_logo = run_command(curl, 
    'https://gitlab.freedesktop.org/wayland/weston/-/raw/main/data/wayland.svg?ref_type=heads&inline=false', 
    '-o', meson.project_build_root() / 'wayland.svg', 
    check: false
  )
  if wayland_logo.returncode() == 0
    install_data(meson.project_build_root() / 'wayland.svg', install_dir: get_option('datadir') / meson.project_name())
  endif
endif

exec = executable(meson.project_name(), 
  ['main.c', 'wayland.c', 'draw.c', 'util.c', xdg_shell_code, xdg_shell_header], 
  dependencies:[wldep, wayland_protos, wl_cursor_dep, plutosvg_dep],
  install: true,
)

subdir('doc')
subdir('completion')
