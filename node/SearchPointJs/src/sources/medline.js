let syncreq = require('sync-request');

class MedlineDataSource {
    constructor(opts) {
        let self = this;

        let config = opts.config;

        self._log = opts.log;
        self._host = config.host;
        self._port = config.port;
    }

    fetchData(query, limit) {
        let self = this;
        let log = self._log;

        try {
            let url = `http://${self._host}:${self._port}/pubmedarticleset008/_search?q=${query}&from=0&size=${limit}`;
            let resp = syncreq(url);

            if (resp.statusCode != 200) {
                log.warn('Received non-OK status code: %d', resp.statusCode);
                log.warn('Response:\n%s', resp.body.toString());
            }

            return self._parseResp(resp);
        } catch (e) {
            log.error(e, 'Failed to execute query, returning NULL!');
            return null
        }
    }

    _parseResp(dataStr) {
        let data = JSON.parse(dataStr);
        let items = data.hits.hits;

        let result = [];
        for (let itemN = 0; itemN < items.length; itemN++) {
            let item = items[itemN];

            let itemId = item.PMID;
            let title = item._source.ArticleTitle;
            let abstract = item._source.Abstract;
            let itemUrl = 'https://www.ncbi.nlm.nih.gov/pubmed/' + itemId;

            result.push({
                title: title,
                description: abstract,
                url: itemUrl,
                displayUrl: itemUrl
            })
        }

        return result;
    }
}

module.exports = exports = function (opts) {
    let source = new MedlineDataSource(opts);
    return function (query, limit) {
        return source.fetchData(query, limit);
    }
}
