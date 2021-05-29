project('picard', ['c'],
	version: '0.0.1',
	default_options: [ 'c_std=c11' ],
)

cc = meson.get_compiler('c')

add_project_arguments('-DPROJECT_VERSION="'+meson.project_version()+'"', language : ['c', 'cpp'])
add_project_arguments('-DPROJECT_NAME="'+meson.project_name()+'"', language : ['c', 'cpp'])

'''
foreach a: [ '-fanalyzer' ]
	if cc.has_argument(a)
		add_project_arguments(a, language : 'c')
	endif
endforeach
'''

foreach a : [ '-ffunction-sections', '-Wshadow', '-Wsign-compare' ]
	if cc.has_argument(a)
		add_project_arguments(a, language : 'c')
	endif
endforeach

foreach a : [ '-Wl,--gc-sections' ]
	if cc.has_link_argument(a)
		add_project_link_arguments(a, language : 'c')
	endif
endforeach

lm = cc.find_library('m', required: false)

subdir('src')
includes = include_directories('src')

picard = executable(
	'picard',
	pid_sources,
	include_directories: includes,
	dependencies: lm,
	install: true,
)
