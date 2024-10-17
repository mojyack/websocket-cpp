ws_files = files(
  'impl.cpp',
  'misc.cpp',
)

ws_server_files = files(
  'server-common.cpp',
  'server.cpp',
)

ws_client_files = files(
  'client.cpp',
)

ws_deps = [
  dependency('libwebsockets', version : '>=4.3.2'), # TODO: investigate minimal version, 4.0.20 not works
]

if host_machine.system() == 'windows'
  cpp = meson.get_compiler('cpp')
  ws_deps += [
    cpp.find_library('ws2_32'),
    cpp.find_library('Crypt32'),
  ]
endif
