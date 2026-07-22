project('waylogo', 'c', version:'0.01')

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

plutosvg_dep = dependency('plutosvg', required: false)

if not plutosvg_dep.found()
  plutosvg_proj = subproject('plutosvg')
  plutosvg_dep = plutosvg_proj.get_variable('plutosvg_dep')
endif

executable(meson.project_name(), ['main.c', xdg_shell_code, xdg_shell_header], dependencies:[wldep, wayland_protos, plutosvg_dep])
