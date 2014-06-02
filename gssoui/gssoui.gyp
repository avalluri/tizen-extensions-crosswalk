{
  'includes':[
    '../common/common.gypi',
  ],
  'variables': {
    'gdbus_codegen_path': '<(SHARED_INTERMEDIATE_DIR)/gssoui',
  },
  'targets': [
    {
      'target_name': 'tizen_gssoui',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'glib-2.0',
          'gio-2.0',
          'gio-unix-2.0',
          'libsoup-2.4',
        ],
      },
      'includes': [
        '../common/pkg-config.gypi',
      ],
      'dependencies': [
        'tizen_gssoui_gen',
      ],
      'sources': [
        'gssoui_api.js',
        'gssoui_data.cc',
        'gssoui_data.h',
        'gssoui_extension.cc',
        'gssoui_extension.h',
        'gssoui_instance.cc',
        'gssoui_instance.h',
        'gssoui_logs.h',
        'gssoui_server.cc',
        'gssoui_server.h',
      ],
    },
    {
      'target_name': 'tizen_gssoui_gen',
      'type': 'static_library',
      'variables': {
        'packages': [
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'include_dirs': [
        './',
      ],
      'actions': [
        {
          'variables': {
            'generate_args': [
              '--interface-prefix',
              'com.google.code.AccountsSSO.gSingleSignOn',
              '--c-namespace',
              'GSSO_DBus',
              '--generate-c-code',
              '<(gdbus_codegen_path)/gssoui_dbus_glue',
            ],
          },
          'action_name': 'gssoui_gen',
          'inputs': [
            'com.google.code.AccountsSSO.UI.xml',
          ],
          'outputs': [
            '<(gdbus_codegen_path)/gssoui_dbus_glue.c',
            '<(gdbus_codegen_path)/gssoui_dbus_glue.h',
          ],
          'action': [
            'gdbus-codegen',
            '<@(generate_args)',
            '<@(_inputs)',
          ],
        },
      ],
      'cflags!': [ '-std=c++0x' ],
      'sources': [
        '<(gdbus_codegen_path)/gssoui_dbus_glue.c',
        '<(gdbus_codegen_path)/gssoui_dbus_glue.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}
