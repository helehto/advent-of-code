project('sub1sec', 'cpp', default_options: ['cpp_std=c++20'])

add_project_arguments('-march=native', language: 'cpp')
add_project_arguments('-fno-omit-frame-pointer', language: 'cpp')

days = [
  [2015, 1],
  [2015, 2],
  [2015, 3],
  [2015, 4],
  [2015, 5],
  [2015, 6],
  [2015, 8],
  [2022, 1],
  [2022, 2],
  [2022, 3],
  [2022, 4],
  [2022, 5],
  [2022, 6],
  [2022, 7],
  [2022, 8],
  [2022, 9],
  [2022, 10],
  [2022, 11],
  [2022, 12],
  [2022, 13],
  [2022, 14],
  [2022, 15],
  [2022, 16],
  [2022, 17],
  [2022, 18],
  [2022, 19],
  [2022, 20],
  [2022, 21],
  [2022, 22],
  [2022, 23],
  [2022, 24],
  [2022, 25],
]

libfmt = dependency('fmt')

sources = []
xmacro_body = ''
foreach d : days
  sources += ['@0@/@1@.cc'.format(d[0], d[1])]
  xmacro_body = xmacro_body + '    X(@0@, @1@) \\\n'.format(d[0], d[1])
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
  dependencies: libfmt,
)
