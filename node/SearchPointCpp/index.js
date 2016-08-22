var sp = require('bindings')('sp.node');
var httpsync = require('http-sync');
var math = require('mathjs');
var util = require('util');
var bunyan = require('bunyan');
var logformat = require('bunyan-format');

function initLog(config) {
	if (config == null) {
		return bunyan.createLogger({ 
			name: 'SearchPointMain',
			stream: logformat({ 
				outputMode: 'short',
				out: process.stdout
			}),
			level: 'info'
		});
	} else {
		return bunyan.createLogger(config);
	}
}

module.exports = function (logConfig) {
	// initialize the logger
	var log = initLog(logConfig);
	
	log.info('Initializing module SearchPoint ...');
	
	// BING data source
	(function () {
		log.info('Initializing BING data source ...');
		
		var apiKeys = [
			"IWBf7jPfsFw7m2QrNM493NWYTYiJ0ynYXVejNWA6kkc",
			"Y8O7wAHCl7z/DIlISSbZbAPDGT7waINUCSMC89gAHGA=",
			"agzOfae9TB8CsLkJ8rxSN/fq5QjG4G1H+RW+JzYMnbY",
			'cFxO0RVY/YrD2pOjwAFL1izVwrFCCNEFneel599YtaI='
		];
		
		if (apiKeys == null || apiKeys.length == 0) {
			log.fatal('Bing API keys missing! Exiting ...');
			process.exit(3);
		}
		
		var MAX_PER_QUERY = 50;
		
		for (var i = 0; i < apiKeys.length; i++) {
			var key = apiKeys[i];
			var encoded = new Buffer(':' + key).toString('base64');	
			apiKeys[i] = encoded;
		}
		
		function parseBingResp(data) {		
			try {
				if (log.trace())
					log.trace('Parsing BING response:\n%s', data);
				
				var dataJson = JSON.parse(data);
				var result = [];
				
				dataJson.d.results.forEach(function (val) {
					result.push({
						title: val.Title,
						description: val.Description,
						url: val.Url,
						displayUrl: val.DisplayUrl
					})
				});
				
				return { items: result, hasNext: '__next' in dataJson.d };
			} catch (e) {
				console.log(e.message);
			}
			
			return null;
		}
		
		sp.fetchBing = function (query, limit) {
			if (log.debug())
				log.debug('Fetching results for query \'%s\' with limit %d', query, limit);
			
			var result = [];
			var nQueries = Math.ceil(limit / MAX_PER_QUERY);
			
			if (log.debug())
				log.debug('Total number of queries: %d', nQueries);
			
			var hasNext = true;
			var offset = 0;
			var i = 0;
			while (hasNext && i++ < nQueries) {
				try {
					// select the API key
					var apiKey = apiKeys[math.randomInt(apiKeys.length)];
					var top = Math.min(limit - offset, MAX_PER_QUERY);
					
					if (log.debug())
						log.debug('Processing query %d, offset: %d, limit: %d', i, offset, top);
					
					var opts = {
						host: 'api.datamarket.azure.com',
						path: '/Data.ashx/Bing/SearchWeb/v1/Web?Query=%27' + query + '%27&$skip=' + offset + '&$top=' + top + '&$format=JSON',
						protocol: 'https',
						method: 'GET',
						headers: {
							'Authorization': 'Basic ' + apiKey
						}
					}
					
					var req = httpsync.request(opts);
					var resp = req.end();
					
					if (resp.statusCode != 200)
						log.warn('Received non-OK status code: %d', resp.statusCode);
					
					var parsed = parseBingResp(resp.body.toString());
					
					if (parsed == null) break;
					
					result = result.concat(parsed.items);
					hasNext = parsed.hasNext;
					
					if (log.debug())
						log.debug('Received %d results!', parsed.items.length);
					
					offset += parsed.items.length;
				} catch (e) {
					log.error(e, 'Failed to execute query!');
					break;
				}
			}
			
			if (log.debug())
				log.debug('Done!');
					
			return result;
		}
		
		log.info('Done!');
	})()
	
	return sp;
}

//module.exports = exports = sp;