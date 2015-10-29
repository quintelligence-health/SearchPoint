{
    'target_defaults': {
        'default_configuration': 'Release',
        'configurations': {
            'Debug': {
                'defines': [
                    'DEBUG',
                ],
            },
            'Release': {
                'defines': [
                    'NDEBUG'
                ],
            }
        },
        'defines': [
            '<(LIN_ALG_BLAS)',
            '<(LIN_ALG_LAPACKE)',
            '<(INDEX_64)',
            '<(INTEL)'
        ],
        # hack for setting xcode settings based on example from
        # http://src.chromium.org/svn/trunk/o3d/build/common.gypi
        'target_conditions': [        
            ['OS=="mac"', {
                'xcode_settings': {
                    'MACOSX_DEPLOYMENT_TARGET': '10.7',
                    'GCC_ENABLE_CPP_RTTI': 'YES',
                    'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                    'OTHER_CFLAGS': [ '-std=c++11', '-stdlib=libc++' ]
                },
            }],
        ],
        'conditions': [
            # operating system specific parameters
            ['OS == "linux"', {
                'libraries': [ '-lrt', '-luuid', '-fopenmp', '<(LIN_ALG_LIB)' ],
                # GCC flags
                'cflags_cc!': [ '-fno-rtti', '-fno-exceptions' ],
                'cflags_cc': [ '-std=c++0x', '-frtti', '-fexceptions' ],
                'cflags': [ '-Wno-deprecated-declarations', '-fopenmp' ]
            }],
            ['OS == "win"', {
                'msbuild_toolset': 'v120',
                'msvs_settings': {
                    'VCCLCompilerTool': {
                        #'RuntimeTypeInfo': 'true',      # /GR  : this should work but doesn't get picked up
                        #'ExceptionHandling': 1,         # /EHsc: this should work but doesn't get picked up
                        'OpenMP': 'true',
                        "AdditionalOptions": [ "/EHsc /GR" ] # release mode displays D9025 warnings, which is a known issue https://github.com/nodejs/node-gyp/issues/335
                    },
                    'VCLinkerTool': {
                        'SubSystem' : 1, # Console
                        'AdditionalOptions': ['<(LIN_ALG_LIB)']
                    },
                },
            }],
            ['OS == "mac"', {
                "default_configuration": "Release",
                "configurations": {
                    "Debug": {
                        "defines": [
                            "DEBUG",
                        ],
                        "xcode_settings": {
                            "GCC_OPTIMIZATION_LEVEL": "0",
                            "GCC_GENERATE_DEBUGGING_SYMBOLS": "YES"
                        }
                    },
                    "Release": {
                        "defines": [
                            "NDEBUG"
                        ],
                        "xcode_settings": {
                            "GCC_OPTIMIZATION_LEVEL": "3",
                            "GCC_GENERATE_DEBUGGING_SYMBOLS": "NO",
                            "DEAD_CODE_STRIPPING": "YES",
                            "GCC_INLINES_ARE_PRIVATE_EXTERN": "YES"
                        }
                    }
                }
            }]
        ],
    },
    'targets': [
        {
            # node SearchPoint module
            'target_name': 'sp',
            'sources': [
            	'src/spnode.h',
            	'src/spnode.cpp',
                'src/sp.h',
                'src/sp.cpp',
                '../../../qminer/src/nodejs/nodeutil.h',
                '../../../qminer/src/nodejs/nodeutil.cpp'
            ],
            'include_dirs': [
                'src/',
                '../../../qminer/src/nodejs/',
                '../../../qminer/src/glib/base/',
                '../../../qminer/src/glib/mine/',
                '../../../qminer/src/glib/misc/',
                '../../../qminer/src/glib/concurrent/'
            ],
            'defines': [
            ],
            'dependencies': [
                'glib'
            ],
            'conditions': [
                # operating system specific parameters
                ['OS == "linux"', { 'libraries': [ '-lrt', '-luuid', '-fopenmp', '-llapacke' ]}],
                ['OS == "mac"', {
                    'xcode_settings': {
                        'MACOSX_DEPLOYMENT_TARGET': '10.7',
                        'GCC_ENABLE_CPP_RTTI': 'YES',
                        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
                        'OTHER_CFLAGS': [ '-std=c++11', '-stdlib=libc++' ],
                        'OTHER_LDFLAGS': [ '-undefined dynamic_lookup' ]
                    }
                }]
            ]
        }, {
            # glib library
            'target_name': 'glib',
            'type': 'static_library',
            'sources': [
                '../../../qminer/src/glib/base/base.h',
                '../../../qminer/src/glib/base/base.cpp',
                '../../../qminer/src/glib/mine/mine.h',
                '../../../qminer/src/glib/mine/mine.cpp',
                '../../../qminer/src/glib/concurrent/thread.h',
                '../../../qminer/src/glib/concurrent/thread.cpp'
            ],        
            'include_dirs': [
                '../../../qminer/src/glib/base/',
                '../../../qminer/src/glib/mine/',
                '../../../qminer/src/glib/misc/',
                '../../../qminer/src/glib/concurrent/'
            ],
            'defines': [
            ]
        }
    ]
}
