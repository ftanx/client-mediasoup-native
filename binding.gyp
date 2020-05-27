{
  "targets": [
    {
      "target_name": "addon",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "WEBRTC_MAC", "WEBRTC_POSIX" ],
      "sources": [
        "src_cpp/addon.cc",
        "src_cpp/main.cpp",
        "src_cpp/MediaStreamTrackFactory.cpp",
        "src_cpp/fakegenerators/fake_audio_capture_module.cc",
        "src_cpp/fakegenerators/fake_frame_source.cc",
        "src_cpp/fakegenerators/frame_generator_capturer.cc",
        "src_cpp/fakegenerators/frame_generator.cc",
        "src_cpp/fakegenerators/test_video_capturer.cc",
        "src_cpp/fakegenerators/frame_utils.cc",
        "src_cpp/fakegenerators/task_queue_for_test.cc"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "include",
        "deps/cpr/include",
        "deps/libmediasoupclient/include",
        "deps/json-develop/single_include/nlohmann",
        "/Users/tobias/Developer/mediasoup/webrtc-checkout/src",
        "/usr/local/Cellar/openssl@1.1/1.1.1g/include"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "conditions" : [
        [
          'OS=="mac"', {
                'xcode_settings': {
                  'GCC_ENABLE_CPP_RTTI': 'YES',
                  'MACOSX_DEPLOYMENT_TARGET': '10.7',
                  'OTHER_CPLUSPLUSFLAGS': [
                    '-Wall',
                    '-Wextra',
                    '-Wpedantic',
                    '-std=c++14',
                    '-stdlib=libc++',
                    '-fexceptions'
                  ],
                  'OTHER_LDFLAGS': [
                    "-Wl,-rpath,<@(module_root_dir)/build/Release"
                  ]
                },
          },
        ],
        [
          'OS=="win"', {
          },
        ],
        [
          'OS=="linux"', {
          }
        ]
      ]
    }
  ]
}
