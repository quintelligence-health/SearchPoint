var sp = require('bindings')('sp.node');
// var httpsync = require('http-sync');
var syncrequest = require('sync-request');
var math = require('mathjs');
var util = require('util');
var bunyan = require('bunyan');
var logformat = require('bunyan-format');
var querystring = require('querystring');

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

        // var apiKeys = [
        //     'a024f31decb74437879f00aa568a23d6'
        // ]
        var apiKeys = [
            '8cfb996728cb4366b146c54c0a3cf93e'      // the one registered on Mateja's name
        ]
        // var apiKeys = [ // base 64
        //     'YTAyNGYzMWRlY2I3NDQzNzg3OWYwMGFhNTY4YTIzZDY='
        // ]
        // var apiKeys = [
        //  "IWBf7jPfsFw7m2QrNM493NWYTYiJ0ynYXVejNWA6kkc",
        //  "Y8O7wAHCl7z/DIlISSbZbAPDGT7waINUCSMC89gAHGA=",
        //  "agzOfae9TB8CsLkJ8rxSN/fq5QjG4G1H+RW+JzYMnbY",
        //  'cFxO0RVY/YrD2pOjwAFL1izVwrFCCNEFneel599YtaI='
        // ];

        if (apiKeys == null || apiKeys.length == 0) {
            log.fatal('Bing API keys missing! Exiting ...');
            process.exit(3);
        }

        log.info('Using %d BING API keys ...', apiKeys.length);

        var MAX_PER_QUERY = 50;

        // for (var i = 0; i < apiKeys.length; i++) {
        //  var key = apiKeys[i];
        //  var encoded = new Buffer(':' + key).toString('base64');
        //  apiKeys[i] = encoded;
        // }

        function parseBingResp(data) {
            try {
                var dataJson = JSON.parse(data);
                var result = [];

                if (log.trace())
                    log.trace('Parsing BING response:\n%s', JSON.stringify(dataJson, null, ' '));

                var totalEstimated = dataJson.webPages.totalEstimatedMatches;

                dataJson.webPages.value.forEach(function (val) {
                    result.push({
                        title: val.name,
                        description: val.snippet,
                        url: val.url,
                        displayUrl: val.displayUrl
                    })
                });

                if (log.debug())
                    log.debug('Received %d of %d results!', result.length, totalEstimated);

                return { items: result, totalEstimated: totalEstimated };
            } catch (e) {
                console.log(e.message);
            }

            return null;
        }

        sp.fetchBing = function (query, limit) {
            if (log.debug())
                log.debug('Fetching results for query \'%s\' with limit %d', query, limit);

            var result = [];
            // var nQueries = Math.ceil(limit / MAX_PER_QUERY);

            // if (log.debug())
            //  log.debug('Total number of queries: %d', nQueries);

            var hasNext = true;
            var offset = 0;
            var i = 0;
            while (hasNext && offset < limit) {
                try {
                    // select the API key
                    var keyN = math.randomInt(apiKeys.length);

                    if (log.debug())
                        log.debug('Using API key number %d ...', keyN);

                    var apiKey = apiKeys[keyN];
                    var top = Math.min(limit - offset, MAX_PER_QUERY);

                    if (log.debug())
                        log.debug('Processing query %d, offset: %d, limit: %d, apiKey: %s', i, offset, top, apiKey);

                    // var path = '/bing/v5.0/search?q=%27' + query + '%27&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';
                    // var path = '/bing/v5.0/search?q=' + querystring.escape(query) + '&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';
                    var path = '/bing/v5.0/search?q=%27' + query + '%27&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';

                    if (log.debug())
                        log.debug('Using path: ' + path);

                    var opts = {
                        headers: {
                            'Ocp-Apim-Subscription-Key': apiKey
                        }
                    }

                    var resp = syncrequest('GET', 'https://api.cognitive.microsoft.com' + path, opts);

                    if (resp.statusCode != 200) {
                        log.warn('Received non-OK status code: %d', resp.statusCode);
                        log.warn('Response:\n%s', resp.body.toString());
                    }

                    var parsed = parseBingResp(resp.body.toString());

                    if (parsed == null) break;

                    if (offset + parsed.items.length > limit) {
                        parsed.items = parsed.items.slice(0, limit - offset);
                    }

                    result = result.concat(parsed.items);
                    hasNext = offset + parsed.items.length < parsed.totalEstimated;
                    // hasNext = parsed.hasNext;

                    if (log.debug())
                        log.debug('Received %d results of %d, hasNext: ' + hasNext + '!', parsed.items.length, parsed.totalEstimated);

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
