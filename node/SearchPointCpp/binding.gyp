{
    'target_defaults': {
        # GCC flags
        'cflags_cc!': [ '-fno-rtti', '-fno-exceptions' ],
        'cflags_cc': [ '-std=c++0x', '-frtti', '-fexceptions' ],
        'cflags': [ '-g', '-fexceptions', '-frtti', '-Wall', '-Wno-deprecated-declarations', '-fopenmp' ]
    },
    'targets': [
        {
            # node qminer module
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
                'OPENBLAS' 
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
        }
    ]
}
