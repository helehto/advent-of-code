project('sub1sec', 'cpp', default_options: ['cpp_std=c++20'])

add_project_arguments('-march=native', language: 'cpp')

if get_option('buildtype') == 'debug'
  # AVX-512 breaks Valgrind.
  add_project_arguments('-mno-avx512f', language: 'cpp')
endif

add_project_arguments('-fno-omit-frame-pointer', language: 'cpp')
add_project_arguments('-fno-exceptions', language: 'cpp')
add_project_arguments('-Wno-stringop-overflow', language: 'cpp')

solutions = [
  [2015, [1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 17, 18]],
  [2021, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14]],
  [2022, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]],
  [2023, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25]],
]

eigen3 = dependency('eigen3')
libfmt = dependency('fmt')

sources = []
xmacro_body = ''
foreach p : solutions
  year = p[0]
  foreach day : p[1]
    sources += ['@0@/@1@.cc'.format(year, day)]
    xmacro_body = xmacro_body + '    X(@0@, @1@) \\\n'.format(year, day)
  endforeach
endforeach

config_h = configure_file(
  input: files('config.h.in'),
  output: 'config.h',
  configuration: {'PROBLEMS': xmacro_body}
)

executable(
  'aoc',
  sources,
  config_h,
  'main.cc',
  dependencies: [
    eigen3,
    libfmt,
  ]
)
