project(
  'aoc-sub1sec',
  'cpp',
  default_options: [
    'cpp_std=c++23',
    'buildtype=release',
    'b_ndebug=if-release',
    'debug=true',
    'warning_level=3',
  ],
)

solutions = {
  '2015': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2016': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2017': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2018': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21],
  '2019': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 24, 25],
  '2020': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 21, 22, 24],
  '2021': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20, 21, 22, 24, 25],
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

extra_warning_flags = []
if get_option('uberpedantic')
  foreach line : import('fs').read('meson-warning-flags.txt').split('\n')
    line = line.strip()
    if line == '' or line.startswith('#')
      continue
    endif

    if line.contains(':')
      fields = line.split(':')
      if fields[0] != cpp.get_id()
        continue
      endif
      line = fields[1].strip()
    endif

    extra_warning_flags += line
  endforeach
endif

cpp_args = [
  # Try to make use of anything that the CPU supports for optimization.
  '-march=native',

  # Omitting the frame pointer doesn't matter much performance-wise in our case
  # (±0.2%), but makes debugging release builds more painful than it has to be.
  '-fno-omit-frame-pointer',

  # We don't use exceptions or RTTI.
  #'-fno-exceptions',
  #'-fno-rtti',

 cpp.get_supported_arguments(
   extra_warning_flags,

    # Warnings that we don't care about:
    '-Wno-missing-field-initializers',
    '-Wno-pedantic',
    '-Wno-unknown-pragmas',
    '-Wno-unknown-warning-option',

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

link_args = [
  # mold is faster than standard GNU ld; use it if possible.
  cpp.get_supported_link_arguments('-fuse-ld=mold'),
]

executable(
  'aoc',
  sources,
  config_h,
  'main.cc',
  cpp_args: cpp_args,
  dependencies: [
    dependency('eigen3', include_type: 'system'),
    dependency('fmt'),
  ],
  include_directories: include_directories('include'),
  link_args: link_args,
)

doctest = dependency('doctest', required: false, disabler: true)
test('aoc-tests', executable(
  'aoc-tests',
  'tests/small_vector.cc',
  cpp_args: [
    cpp_args,
    '-mno-avx512f',
    '-DDOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS',
  ],
  dependencies: [doctest, dependency('fmt')],
  include_directories: include_directories('include'),
  link_args: link_args,
))
