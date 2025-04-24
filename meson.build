project(
  'aoc-sub1sec',
  'cpp',
  default_options: [
    'cpp_std=c++23',
    'buildtype=release',
    'b_ndebug=if-release',
    'debug=true',
  ],
)

solutions = {
  '2015': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21],
  '2016': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 19, 20, 21, 22, 23, 24],
  '2017': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2018': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16],
  '2019': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25],
  '2020': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 22, 24],
  '2021': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20],
  '2022': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2023': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2024': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 22, 25],
}

cpp = meson.get_compiler('cpp')

sources = []
xmacro_body = ''
foreach year, days : solutions
  foreach day : days
    sources += ['@0@/@1@.cc'.format(year, day)]
    xmacro_body = xmacro_body + '    X(@0@, @1@) \\\n'.format(year, day)
  endforeach
endforeach

config_h = configure_file(
  input: files('include/config.h.in'),
  output: 'config.h',
  configuration: {'PROBLEMS': xmacro_body}
)

cpp_args = [
  # Try to make use of anything that the CPU supports for optimization.
  '-march=native',

  # Omitting the frame pointer doesn't matter much performance-wise in our case
  # (±0.2%), but makes debugging release builds more painful than it has to be.
  '-fno-omit-frame-pointer',

  # We don't use exceptions or RTTI.
  '-fno-exceptions',
  '-fno-rtti',

  cpp.get_supported_arguments(
    # Warnings that we don't care about:
    '-Wno-missing-field-initializers',
    '-Wno-pedantic',
    # False positives which are hard to squelch:
    '-Wno-stringop-overflow',
  ),

  # _GLIBCXX_DEBUG makes things drastically slower and changes the ABI, so it
  # is completely opt-in:
  get_option('glibcxx_debug') ? '-D_GLIBCXX_DEBUG' : [],

  # AVX-512 breaks Valgrind. Disable it for debug builds, where we might want
  # to use it, but not in release builds where we want the compiler to
  # autovectorize stuff as well as it can.
  get_option('buildtype') == 'debug' ? '-mno-avx512f' : [],
]

executable(
  'aoc',
  sources,
  config_h,
  'main.cc',
  cpp_args: cpp_args,
  dependencies: [dependency('eigen3'), dependency('fmt')],
  include_directories: include_directories('include'),
  # mold is faster than standard GNU ld; use it if possible.
  link_args: cpp.get_supported_link_arguments('-fuse-ld=mold'),
)
