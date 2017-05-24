let math = require('mathjs');
let syncrequest = require('sync-request');

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

    fetchData(query, limit) {
        let self = this;
        let log = self._log;

        if (log.debug())
            log.debug('Fetching results for query \'%s\' with limit %d', query, limit);

        let result = [];
        // let nQueries = Math.ceil(limit / MAX_PER_QUERY);

        // if (log.debug())
        //  log.debug('Total number of queries: %d', nQueries);

        let hasNext = true;
        let offset = 0;
        let i = 0;
        while (hasNext && offset < limit) {
            try {
                let apiKeys = self._apiKeys;
                // select the API key
                let keyN = math.randomInt(apiKeys.length);

                if (log.debug())
                    log.debug('Using API key number %d ...', keyN);

                let apiKey = apiKeys[keyN];
                let top = Math.min(limit - offset, self._MAX_PER_QUERY);

                if (log.debug())
                    log.debug('Processing query %d, offset: %d, limit: %d, apiKey: %s', i, offset, top, apiKey);

                // let path = '/bing/v5.0/search?q=%27' + query + '%27&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';
                // let path = '/bing/v5.0/search?q=' + querystring.escape(query) + '&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';
                let path = '/bing/v5.0/search?q=%27' + query + '%27&count=' + top + '&offset=' + offset + '&mkt=en-us&safesearch=Moderate';

                if (log.debug())
                    log.debug('Using path: ' + path);

                let opts = {
                    headers: {
                        'Ocp-Apim-Subscription-Key': apiKey
                    }
                }

                let resp = syncrequest('GET', 'https://api.cognitive.microsoft.com' + path, opts);

                if (resp.statusCode != 200) {
                    log.warn('Received non-OK status code: %d', resp.statusCode);
                    log.warn('Response:\n%s', resp.body.toString());
                }

                let parsed = self._parseResponse(resp.body.toString());

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

    _parseResponse(data) {
        let self = this;
        let log = self._log;

        try {
            let dataJson = JSON.parse(data);
            let result = [];

            if (log.trace())
                log.trace('Parsing BING response:\n%s', JSON.stringify(dataJson, null, ' '));

            let totalEstimated = dataJson.webPages.totalEstimatedMatches;

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
}

module.exports = exports = function (opts) {
    let source = new BingDataSource(opts);
    return function (query, limit) {
        return source.fetchData(query, limit);
    }
}
