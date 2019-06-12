let request = require('request');
// let syncreq = require('sync-request');

class MedlineDataSource {
    constructor(opts) {
        let self = this;

        let config = opts.config;

        self._log = opts.log;
        self._host = config.host;
        self._port = config.port;
        self._username = config.username;
        self._password = config.password;
    }

    fetchData(query, limit, callback) {
        let self = this;
        let log = self._log;

        try {
            let url = 'http://' + self._username + ':' +
                self._password + '@' +
                self._host + ':' +
                self._port +
                '/pubmedarticleset019/_search?q=' + query + '&from=0&size=' + limit;

            if (log.debug())
                log.debug('Fetching URL: ' + url);

            request(url, function (e, response, body) {
                if (e != null) return callback(e);

                let status = response.statusCode;
                if (status < 200 || 300 <= status) {
                    return callback(new Error('Status code: ' + status + ' from URL: ' + url));
                }

                let parsed = self._parseResp(body);

//                 return self._parseResp(resp.getBody());
                callback(undefined, parsed);
            })
            // let resp = syncreq('GET', url);

            // if (resp.statusCode != 200) {
            //     log.warn('Received non-OK status code: %d', resp.statusCode);
            //     log.warn('Response:\n%s', resp.body.toString());
            // }
        } catch (e) {
            log.error(e, 'Failed to execute query, returning NULL!');
            callback(undefined, []);
        }
    }

    _parseResp(dataStr) {
        let self = this;
        let log = self._log;

        if (log.debug())
            log.debug('parsing response');

        let data = JSON.parse(dataStr);
        let items = data.hits.hits;

        let result = [];
        for (let itemN = 0; itemN < items.length; itemN++) {
            let item = items[itemN];

            let itemId = item._source.PMID;
            let title = item._source.ArticleTitle;
            let description = item._source.Abstract;
            let itemUrl = 'https://www.ncbi.nlm.nih.gov/pubmed/' + itemId;

            if (description == null) { description = title; }

            let transformed = {
                title: title,
                description: description,
                url: itemUrl,
                displayUrl: itemUrl
            }

            if (log.trace())
                log.trace('transformed item:\n' + JSON.stringify(transformed, null, ' '));

            result.push(transformed);
        }

        if (log.debug())
            log.debug('response parsed');

        return result;
    }
}

module.exports = exports = function (opts) {
    let source = new MedlineDataSource(opts);
    return function (query, limit, callback) {
        return source.fetchData(query, limit, callback);
    }
}
