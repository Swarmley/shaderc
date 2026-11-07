use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '5be22f98733c674d532598454ae729253bc53e82',
  'effcee_revision' : '05be15d8a2b985a3ffee004a5e1fcea8a0528d0a',
  'glslang_revision': '83f75db6ada23801a65eed1cde4763a0a5bfbda6',
  'googletest_revision': '6450691fd7d07ef932582206fbafb1a513927e08',
  're2_revision': '2da0056814cf180480a19f5cf811e7e1c054bf6d',
  'spirv_headers_revision': '6efe5ae6b7c6ad29da3f2cc3f164b9c3d72b78b1',
  'spirv_tools_revision': 'b570f00a782a7bf4006964e8284dd8c10c4c549b',
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
