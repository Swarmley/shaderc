use_relative_paths = True

vars = {
  'abseil_git':  'https://github.com/abseil',
  'google_git':  'https://github.com/google',
  'khronos_git': 'https://github.com/KhronosGroup',

  'abseil_revision': '5be22f98733c674d532598454ae729253bc53e82',
  'effcee_revision' : 'f8e8a164822d4f65e757bff66bc00e1567959aa0',
  'glslang_revision': '5494791363451eb51b959544728c8d204b567fd2',
  'googletest_revision': '4267679b6887f349f17b01ccd70c9e3483689b25',
  're2_revision': '972a15cedd008d846f1a39b2e88ce48d7f166cbd',
  'spirv_headers_revision': '2b7475f6d664efbad4d4315c94ecec9640831e5b',
  'spirv_tools_revision': 'ba9a9e32160e2500ce1e48cc563a8fedaf596e6e',
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
