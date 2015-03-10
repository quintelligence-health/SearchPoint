var fs = require('fs');
var bunyan = require('bunyan');
var logformat = require('bunyan-format');
var express = require('express');
var path = require('path');
var spMod;// = require('../SearchPointCpp/');

var API_PATH = '/api';

var QUERY_PARAM = 'q';
var QUERY_ID_PARAM = 'qid';
var CLUST_KEY_PARAM = 'c';
var LIMIT_PARAM = 'n';
var PAGE_PARAM = 'p';
var COORD_X_PARAM = 'x';
var COORD_Y_PARAM = 'y';
var POSITIONS_PARAM = 'pos';

var DEFAULT_LIMIT = 200;


//==================================================================
// INITIALIZATION FUNCTIONS
//==================================================================

function readConfig(fname) {
	var configStr = fs.readFileSync(fname);
	return JSON.parse(configStr);
}

function initLog() {
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
		} else {	// file
			streams.push({
				path: stream.file,
				level: stream.level
			});
		}
	});
	
	return bunyan.createLogger({
		name: 'SearchPoint',
		streams: streams,
	});
}

function initSp() {
	log.info('Initializing SearchPoint ...');
	var unicodePath = config.sp.unicodePath;
	var dmozPath = config.sp.dmozPath;
	
	var opts = {
		dmozPath: dmozPath,
		unicodePath: unicodePath,
		dataSource: spMod.fetchBing
	}
	
	return new spMod.SearchPoint(opts);
}

// initialization
var config = readConfig(process.argv[2]);
var log = initLog();

spMod = require(config.modulePath);

var sp = initSp();

//==================================================================
// SERVER
//==================================================================

var app = express();
var server;

function initRestApi() {
	app.get(API_PATH + '/query', function (req, resp) {
		try {
			var query = req.query[QUERY_PARAM];//encodeURI(req.query[QUERY_PARAM]);
			var clustKey = req.query[CLUST_KEY_PARAM];
			var limit = LIMIT_PARAM in req.query ? parseInt(req.query[LIMIT_PARAM]) : DEFAULT_LIMIT;
			
			if (isNaN(limit)) limit = DEFAULT_LIMIT;
			
			if (log.debug())
				log.debug('Processing query: query: %s, clust: %s, limit: %d ...', query, clustKey, limit);
			
			var result = sp.processQuery(encodeURI(query), clustKey, limit);
			
			if (log.debug())
				log.debug('Done!');
			
			resp.send(result);						
		} catch (e) {
			log.error(e, 'Failed to query keywords!');
			resp.status(500);	// internal server error
		}
		
		resp.end();
	});
	
	app.get(API_PATH + '/rank', function (req, resp) {
		try {
			var queryId = req.query[QUERY_ID_PARAM];
			var page = parseInt(req.query[PAGE_PARAM]);
			var pos = req.query[POSITIONS_PARAM][0];
			
			if (log.debug())
				log.debug('Ranking: queryId: %s, page: %d, pos: %s', queryId, page, JSON.stringify(pos));
			
			resp.send(sp.rankByPos(parseFloat(pos.x), parseFloat(pos.y), page, queryId));
		} catch (e) {
			log.error(e, 'Failed to query keywords!');
			resp.status(500);	// internal server error
		}
		
		resp.end();
	});
	
	app.get(API_PATH + '/keywords', function (req, resp) {
		try {
			var queryId = req.query[QUERY_ID_PARAM];
			var x = parseFloat(req.query[COORD_X_PARAM]);
			var y = parseFloat(req.query[COORD_Y_PARAM]);
			
			if (log.debug())
				log.debug('Fetching keywords queryId: %s, x: %d, y: %d ...', queryId, x, y);
			
			resp.send(sp.fetchKeywords(x, y, queryId));
		} catch (e) {
			log.error(e, 'Failed to query keywords!');
			resp.status(500);	// internal server error
		}
		
		resp.end();
	});
}

//serve static directories on the UI path
app.use('/', express.static(path.join(__dirname, './ui')));

initRestApi();

// start server
server = app.listen(config.server.port);

log.info('================================================');
log.info('Server running at http://localhost:%d', config.server.port);
log.info('================================================');