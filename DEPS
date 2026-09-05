use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '5be22f98733c674d532598454ae729253bc53e82',
  'effcee_revision' : 'f8e8a164822d4f65e757bff66bc00e1567959aa0',
  'glslang_revision': 'efa016659ffc4f2ae566b6b1db71a70655ac33a1',
  'googletest_revision': '283c17563fe7a1111cd7f581aa5d541e8baeff2f',
  're2_revision': 'c9cba76063cf4235c1a15dd14a24a4ef8d623761',
  'spirv_headers_revision': '124a9665e464ef98b8b718d572d5f329311061eb',
  'spirv_tools_revision': '8e3da01b45806fbacbb9e6fce9c5f9ae49f60e42',
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
