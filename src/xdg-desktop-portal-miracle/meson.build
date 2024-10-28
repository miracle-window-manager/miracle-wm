project(
	'xdg-desktop-portal-wlr',
	'c',
	version: '0.7.1',
	license: 'MIT',
	meson_version: '>=0.60.0',
	default_options: ['c_std=c11', 'warning_level=2', 'werror=true'],
)

cc = meson.get_compiler('c')

add_project_arguments(cc.get_supported_arguments([
	'-Wno-missing-braces',
	'-Wno-missing-field-initializers',
	'-Wno-unused-parameter',
]), language: 'c')

prefix = get_option('prefix')
sysconfdir = get_option('sysconfdir')
add_project_arguments([
	'-D_POSIX_C_SOURCE=200809L',
	'-DSYSCONFDIR="@0@"'.format(prefix / sysconfdir),
], language: 'c')

inc = include_directories('include')

rt = cc.find_library('rt')
pipewire = dependency('libpipewire-0.3', version: '>= 0.3.62')
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.24')
iniparser = dependency('inih')
gbm = dependency('gbm')
drm = dependency('libdrm')

epoll = dependency('', required: false)
if not cc.has_function('timerfd_create', prefix: '#include <sys/timerfd.h>')
	epoll = dependency('epoll-shim')
endif

if get_option('sd-bus-provider') == 'auto'
	assert(get_option('auto_features').auto(), 'sd-bus-provider must not be set to auto since auto_features != auto')
	sdbus = dependency(['libsystemd', 'libelogind', 'basu'])
else
	sdbus = dependency(get_option('sd-bus-provider'))
endif
add_project_arguments('-DHAVE_' + sdbus.name().to_upper() + '=1', language: 'c')

subdir('protocols')

xdpw_files = files(
	'src/core/main.c',
	'src/core/logger.c',
	'src/core/config.c',
	'src/core/request.c',
	'src/core/session.c',
	'src/core/timer.c',
	'src/core/timespec_util.c',
	'src/screenshot/screenshot.c',
	'src/screencast/screencast.c',
	'src/screencast/screencast_common.c',
	'src/screencast/wlr_screencast.c',
	'src/screencast/wlr_screencopy.c',
	'src/screencast/pipewire_screencast.c',
	'src/screencast/fps_limit.c',
)

executable(
	'xdg-desktop-portal-wlr',
	[xdpw_files, wl_proto_files],
	dependencies: [
		wayland_client,
		sdbus,
		pipewire,
		rt,
		iniparser,
		gbm,
		drm,
		epoll,
	],
	include_directories: [inc],
	install: true,
	install_dir: get_option('libexecdir'),
)

conf_data = configuration_data()
conf_data.set('libexecdir', get_option('prefix') / get_option('libexecdir'))
conf_data.set('systemd_service', '')

systemd = dependency('systemd', required: get_option('systemd'))
if systemd.found()
	systemd_service_file = 'xdg-desktop-portal-wlr.service'
	user_unit_dir = systemd.get_variable(pkgconfig: 'systemduserunitdir',
		pkgconfig_define: ['prefix', get_option('prefix')])
	conf_data.set('systemd_service', 'SystemdService=' + systemd_service_file)

	configure_file(
		configuration: conf_data,
		input: 'contrib/systemd/' + systemd_service_file + '.in',
		output: '@BASENAME@',
		install_dir: user_unit_dir,
	)
endif

configure_file(
	configuration: conf_data,
	input: 'org.freedesktop.impl.portal.desktop.wlr.service.in',
	output: '@BASENAME@',
	install_dir: get_option('datadir') / 'dbus-1' / 'services',
)

install_data(
	'wlr.portal',
	install_dir: get_option('datadir') / 'xdg-desktop-portal' / 'portals',
)

scdoc = dependency('scdoc', required: get_option('man-pages'), version: '>= 1.9.7', native: true)
if scdoc.found()
	scdoc_prog = find_program(scdoc.get_variable(pkgconfig: 'scdoc'), native: true)

	man_pages = ['xdg-desktop-portal-wlr.5.scd']
	foreach src : man_pages
		topic = src.split('.')[0]
		section = src.split('.')[1]
		output = topic + '.' + section

		custom_target(
			output,
			input: files(src),
			output: output,
			command: scdoc_prog,
			feed: true,
			capture: true,
			install: true,
			install_dir: get_option('mandir') / ('man' + section),
		)
	endforeach
endif

summary({
	'sd-bus provider': sdbus.name(),
	'systemd service': systemd.found(),
	'Man pages': scdoc.found(),
}, bool_yn: true)
