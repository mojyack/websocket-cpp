project('websocket-cpp', 'cpp', version : '1.0.0', default_options : ['warning_level=3', 'werror=false', 'cpp_std=c++23'])
add_project_arguments('-Wfatal-errors', language : 'cpp')
add_project_arguments('-Wno-missing-field-initializers', language : 'cpp')

subdir('src')

executable('example',
  files(
    'src/example.cpp',
  ) + ws_files + ws_server_files + ws_client_files,
  dependencies : ws_deps,
)
