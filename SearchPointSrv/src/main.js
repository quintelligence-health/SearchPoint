var fs = require('fs');
var bunyan = require('bunyan');
var logformat = require('bunyan-format');

var server = require('./server');
let sp = require('../../SearchPoint/');
let decorations = require('./util/decorations');

function readConfig(fname) {
    var configStr = fs.readFileSync(fname);
    var config = JSON.parse(configStr);
    if (config.source == null) throw new Error('Source config missing!');
    if (config.source.path == null) throw new Error('Source path missing!');
    return config;
}

function initLogStreams() {
    var streams = [];

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
    var config = readConfig(process.argv[2]);
    var logStreams = initLogStreams();
    var log = initLog(logStreams);

    decorations.decorate({
        log: log
    })

    var sourceConfig = config.source;
    var source = require(sourceConfig.path);

    var unicodePath = config.sp.unicodePath;
    var dmozPath = config.sp.dmozPath;

    let searchpoint = new sp.SearchPointStore({
        settings: {
            dmozPath: dmozPath,
            unicodePath: unicodePath
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
    if (log == null) {
        console.error('Exception in main!');
        console.error(e);
    } else {
        log.error(e, 'Unknown exception in main!');
    }
    process.exit(1);
}
