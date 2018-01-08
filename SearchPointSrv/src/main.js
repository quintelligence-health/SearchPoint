let fs = require('fs');
let bunyan = require('bunyan');
let logformat = require('bunyan-format');

let server = require('./server');
let sp = require('../../SearchPoint/');
let decorations = require('./util/decorations');

function readConfig(fname) {
    let configStr = fs.readFileSync(fname);
    let config = JSON.parse(configStr);
    if (config.source == null) throw new Error('Source config missing!');
    if (config.source.path == null) throw new Error('Source path missing!');
    return config;
}

function initLogStreams(config) {
    let streams = [];

    config.logs.forEach(function (stream) {
        if (stream.type == 'stdout') {
            streams.push({
                stream: logformat({
                    outputMode: 'short',
                    out: process.stdout
                }),
                level: stream.level
            });
        } else {    // file
            streams.push({
                path: stream.file,
                level: stream.level
            });
        }
    });

    return streams;
}

function initLog(streams) {
    return bunyan.createLogger({
        name: 'SearchPointMain',
        streams: streams,
    });
}

//==================================================================
// INITIALIZATION FUNCTIONS
//==================================================================

try {
    // initialization
    let config = readConfig(process.argv[2]);
    let logStreams = initLogStreams(config);
    let log = initLog(logStreams);

    decorations.decorate({
        log: log
    })

    let sourceConfig = config.source;
    let source = require(sourceConfig.path);

    let dmozPath = config.sp.dmozPath;

    let searchpoint = new sp.SearchPointStore({
        settings: {
            dmozPath: dmozPath
        },
        timeout: config.server.sessionTimeout,
        log: log,
        cleanup: 'manual'
    })

    server({
        log: log,
        dataSource: source({
            log: log,
            config: sourceConfig.config
        }),
        sp: searchpoint,
        port: config.server.port,
        sessionTimeout: config.server.sessionTimeout
    })
} catch (e) {
    console.error('Exception in main!');
    console.error(e);
    process.exit(1);
}
