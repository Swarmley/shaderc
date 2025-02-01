use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '5be22f98733c674d532598454ae729253bc53e82',
  'effcee_revision' : '12241cbc30f20730b656db7fd5a3fa36cd420843',
  'glslang_revision': '0549c7127c2fbab2904892c9d6ff491fa1e93751',
  'googletest_revision': 'e235eb34c6c4fed790ccdad4b16394301360dcd4',
  're2_revision': '6dcd83d60f7944926bfd308cc13979fc53dd69ca',
  'spirv_headers_revision': 'e7294a8ebed84f8c5bd3686c68dbe12a4e65b644',
  'spirv_tools_revision': 'ce37fd67f83cd1e8793b988d2e4126bbf72b19dd',
}

deps = {
  'third_party/abseil_cpp':
      Var('abseil_git') + '/abseil-cpp.git@' + Var('abseil_revision'),

  'third_party/effcee': Var('google_git') + '/effcee.git@' +
      Var('effcee_revision'),

  'third_party/googletest': Var('google_git') + '/googletest.git@' +
      Var('googletest_revision'),

  'third_party/glslang': Var('khronos_git') + '/glslang.git@' +
      Var('glslang_revision'),

  'third_party/re2': Var('google_git') + '/re2.git@' +
      Var('re2_revision'),

  'third_party/spirv-headers': Var('khronos_git') + '/SPIRV-Headers.git@' +
      Var('spirv_headers_revision'),

  'third_party/spirv-tools': Var('khronos_git') + '/SPIRV-Tools.git@' +
      Var('spirv_tools_revision'),
}
