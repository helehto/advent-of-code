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
  '2016': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24],
  '2017': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25],
  '2018': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18],
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

cpp_args = [
  # Try to make use of anything that the CPU supports for optimization.
  '-march=native',

  # Omitting the frame pointer doesn't matter much performance-wise in our case
  # (±0.2%), but makes debugging release builds more painful than it has to be.
  '-fno-omit-frame-pointer',

  # We don't use exceptions or RTTI.
  #'-fno-exceptions',
  #'-fno-rtti',

  get_option('uberpedantic') ? cpp.get_supported_arguments(
    '-Walloca',
    '-Warray-bounds=2',
    '-Warray-parameter=2',
    '-Wattribute-alias',
    '-Wbidi-chars=any',
    '-Wcast-align',
    '-Wcast-qual',
    '-Wcatch-value=3',
    '-Wchanges-meaning',
    '-Wclass-conversion',
    '-Wcomma-subscript',
    '-Wcomments',
    '-Wconditionally-supported',
    '-Wconversion-null',
    '-Wctor-dtor-privacy',
    '-Wdefaulted-function-deleted',
    '-Wdelete-incomplete',
    '-Wdeprecated-copy-dtor',
    '-Wdeprecated-enum-enum-conversion',
    '-Wdeprecated-enum-float-conversion',
    '-Wdeprecated-literal-operator',
    '-Wdeprecated-variadic-comma-omission',
    '-Wdisabled-optimization',
    '-Wdouble-promotion',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Welaborated-enum-base',
    '-Wexceptions',
    '-Wextra-semi',
    '-Wfloat-equal',
    '-Wformat',
    '-Wformat-nonliteral',
    '-Wformat-overflow=2',
    '-Wformat-security',
    '-Wformat-signedness',
    '-Wformat-truncation=2',
    '-Wformat-y2k',
    '-Wformat=2',
    '-Wglobal-module',
    '-Wimplicit-fallthrough=5',
    '-Winaccessible-base',
    '-Winherited-variadic-ctor',
    '-Winit-list-lifetime',
    '-Winvalid-constexpr',
    '-Winvalid-offsetof',
    '-Winvalid-utf8',
    '-Wliteral-suffix',
    '-Wlogical-op',
    '-Wmismatched-tags',
    '-Wmissing-format-attribute',
    '-Wmissing-include-dirs',
    '-Wmissing-noreturn',
    '-Wmissing-requires',
    '-Wmissing-template-keyword',
    '-Wmultichar',
    '-Wnoexcept',
    '-Wnon-template-friend',
    '-Wnon-virtual-dtor',
    '-Wnormalized',
    '-Wnrvo',
    '-Woverlength-strings',
    '-Woverloaded-virtual=2',
    '-Wpacked',
    '-Wpacked-bitfield-compat',
    '-Wplacement-new=2',
    '-Wpmf-conversions',
    '-Wpointer-arith',
    '-Wshift-overflow=2',
    '-Wsign-promo',
    '-Wstrict-aliasing=3',
    '-Wstrict-null-sentinel',
    '-Wstrict-overflow=1',
    '-Wsuggest-attribute=cold',
    '-Wsuggest-attribute=format',
    '-Wsuggest-attribute=malloc',
    '-Wsuggest-attribute=noreturn',
    '-Wsuggest-attribute=returns_nonnull',
    '-Wsuggest-override',
    '-Wswitch-enum',
    '-Wtemplate-id-cdtor',
    '-Wterminate',
    '-Wtrampolines',
    '-Wtrivial-auto-var-init',
    '-Wunreachable-code',
    '-Wunsafe-loop-optimizations',
    '-Wunused-const-variable=2',
    '-Wunused-macros',
    '-Wunused-parameter',
    '-Wuse-after-free=3',
    '-Wuseless-cast',
    '-Wvariadic-macros',
    '-Wvector-operation-performance',
    '-Wvexing-parse',
    '-Wvirtual-inheritance',
    '-Wvirtual-move-assign',
    '-Wvla',
    '-Wvolatile',
    '-Wwrite-strings',
    '-Wzero-as-null-pointer-constant',
  ) : [],

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
  dependencies: [dependency('eigen3'), dependency('fmt')],
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
