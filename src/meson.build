ws_files = files(
  'impl.cpp',
  'misc.cpp',
)

ws_server_files = files(
  'server.cpp',
)

ws_client_files = files(
  'client.cpp',
)

ws_deps = [
  dependency('libwebsockets', version : '>=4.3.2'), # TODO: investigate minimal version, 4.0.20 not works
]
