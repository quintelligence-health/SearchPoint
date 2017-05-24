var fs = require('fs');
var bunyan = require('bunyan');
var logformat = require('bunyan-format');
var server = require('./src/server.js');

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

function initSp(config) {
    log.info('Initializing SearchPoint ...');
    var spModule = require(config.modulePath);

    log.debug('Configuring data source');
    var sourceConfig = config.source;
    var source = require(sourceConfig.path);

    var unicodePath = config.sp.unicodePath;
    var dmozPath = config.sp.dmozPath;

    var opts = {
        dmozPath: dmozPath,
        unicodePath: unicodePath,
        dataSource: source({
            log: log,
            config: sourceConfig.config
        })
        // dataSource: spModule.fetchBing
    }

    return new spModule.SearchPoint(opts);
}

try {
    // initialization
    var config = readConfig(process.argv[2]);
    var logStreams = initLogStreams();
    var log = initLog(logStreams);

    var sp = initSp(config);
    server({
        log: log,
        sp: sp,
        port: config.server.port
    })
} catch (e) {
    log.error(e, 'Unknown exception in main!');
    process.exit(1);
}
