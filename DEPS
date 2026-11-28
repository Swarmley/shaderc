use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '5be22f98733c674d532598454ae729253bc53e82',
  'effcee_revision' : '05be15d8a2b985a3ffee004a5e1fcea8a0528d0a',
  'glslang_revision': '291d83913ac47c8103daf7138d16bc8072efdce3',
  'googletest_revision': 'a46a6012a133c7c0929fdae55b321c2fe65f3c5f',
  're2_revision': '2da0056814cf180480a19f5cf811e7e1c054bf6d',
  'spirv_headers_revision': 'f6f78c717057ac7f5f3ea021f3f962aaf9d4da7e',
  'spirv_tools_revision': 'b1d2dc272aee6e6c9454d202fe88cbcbbeaab72c',
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
