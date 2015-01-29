{
  'includes':[
    '../common/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'tizen_ping',
      'type': 'loadable_module',
      'variables': {
        'packages': [
          'glib-2.0',
          'gio-2.0',
          'gio-unix-2.0',
        ],
      },
      'sources': [
        'ping_api.js',
        'ping_extension.cc',
        'ping_extension.h',
        'ping_instance.cc',
        'ping_instance.h',
        'ping_logs.h',
      ],
      'includes': [
        '../common/pkg-config.gypi',
      ],
    },
  ],
}
