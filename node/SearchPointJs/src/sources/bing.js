let math = require('mathjs');
let async = require('async');

let utils = require('../util/utils');

class BingDataSource {
    constructor(opts) {
        let self = this;

        self._MAX_PER_QUERY = 50;

        let config = opts.config;

        // checks
        if (config.apiKeys == null || config.apiKeys.length == 0) throw new Error('Did not set API keys!');

        self._log = opts.log;
        self._apiKeys = config.apiKeys;
    }

    fetchData(query, limit, callback) {
        let self = this;
        let log = self._log;

        if (log.debug())
            log.debug('Fetching results for query \'%s\' with limit %d', query, limit);

        let hasNext = true;
        let offset = 0;
        let queryN = 0;
        let batches = [];

        let test = function () {
            return hasNext && offset < limit;
        }

        let fetch = function (done) {
            let apiKeys = self._apiKeys;
            // select the API key
            let keyN = math.randomInt(apiKeys.length);

            if (log.debug())
                log.debug('Using API key number %d ...', keyN);

            let apiKey = apiKeys[keyN];
            let top = Math.min(limit - offset, self._MAX_PER_QUERY);

            if (log.debug())
                log.debug('Processing query %d, offset: %d, limit: %d, apiKey: %s', queryN, offset, top, apiKey);

            // let path = '/bing/v5.0/search?q=%27' + query + '%27&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';
            // let path = '/bing/v5.0/search?q=' + querystring.escape(query) + '&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';
            // let path = '/bing/v5.0/search?q=%27' + query + '%27&count=' + top + '&offset=' + offset + '&';

            // if (log.debug())
            //     log.debug('Using path: ' + path);

            let host = 'api.cognitive.microsoft.com';
            let path = '/bing/v5.0/search';
            let params = {
                q: query,
                count: top,
                offset: offset,
                mkt: 'en-us',
                safesearch: 'Moderate'
            }
            let headers = {
                'Ocp-Apim-Subscription-Key': apiKey
            }

            // let resp = syncrequest('GET', + path, opts);
            utils.httpsGetJson(host, path, params, headers, function (e, itemsJson) {
                if (e != null) return done(e);

                let parsed = self._parseResponse(itemsJson);

                if (parsed == null) return done();

                let batch = parsed.items;
                if (offset + batch.length > limit) {
                    parsed.items = parsed.items.slice(0, limit - offset);
                }

                batches.push(batch);

                if (log.debug())
                    log.debug('Received %d results of %d, hasNext: ' + hasNext + '!', parsed.items.length, parsed.totalEstimated);

                offset += batch.length;
                ++queryN;

                done();
            });

        }

        async.whilst(test, fetch, function (e) {
            if (e != null) return callback(e);

            let result = [].concat(...batches);
            callback(undefined, result);
        })
    }

    _parseResponse(data) {
        let self = this;
        let log = self._log;

        // let data = JSON.parse(data);
        let result = [];

        if (log.trace())
            log.trace('Parsing BING response:\n%s', JSON.stringify(data, null, ' '));

        let totalEstimated = data.webPages.totalEstimatedMatches;

        data.webPages.value.forEach(function (val) {
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
    }
}

module.exports = exports = function (opts) {
    let source = new BingDataSource(opts);
    return function (query, limit, callback) {
        return source.fetchData(query, limit, callback);
    }
}
