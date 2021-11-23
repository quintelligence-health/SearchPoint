let syncreq = require('sync-request');
let {Client} = require('@elastic/elasticsearch');
const fs = require('fs');

class MedlineDataSource {
    constructor(opts) {
        let self = this;

        let config = opts.config;
        let content = fs.readFileSync('config/config-mag-generic.json');
        self.config = JSON.parse(content).meta;
        self._log = opts.log;
        self._host = config.host;
        self._port = config.port;
        self._username = config.username;
        self._password = config.password;
        self.client = new Client({
            node: self._host,
            auth: {
                username: self._username,
                password: self._password
            }
        });
    }

    async fetchData(query, limit, callback) {
        let self = this;
        let log = self._log;
        if (self._host !== "localhost") {
            self._port = "";
        }
        let additionalFilters;
        if (query.additionalFilters){
            additionalFilters=query.additionalFilters;
        }
        let textField = query.textField || "abstract";
        let dateField = query.dateField || "Date";
        let urlField=query.urlField || "paperId";
        let urlPrefix = query.urlPrefix || 'https://academic.microsoft.com/paper/';
        let textSearch = {};
        textSearch[textField] = {"query": query.keyword};
        const withDate= query.date!==undefined;
        let dateRange={};
        if (withDate){
            dateRange[dateField] = {
                "gte": query.date.start,
                "lte": query.date.end
            }
            dateRange={
                "range": dateRange
            };
        }
        const keywordFilter={
            "bool": {
                "should": [
                    {
                        "match": {
                            "title": {
                                "query": query.keyword
                            }
                        }
                    },
                    {
                        "match": textSearch
                    }
                ]
            }
        };
        let totalFilter=withDate?[dateRange,keywordFilter]:[keywordFilter];
        if (additionalFilters){
            totalFilter.push(...additionalFilters)
        }
        const result = await this.client.search({
            index: query.index,
            body: {
                "size": limit,
                "_source": {
                    "exclude": query.excludeFields || []
                },
                "query": {
                    "bool": {
                        "filter": {
                            "bool": {
                                "must": totalFilter
                            }
                        }
                    }
                }
            }
        });

        let res = result.body.hits.hits.filter((hit) => {
            return hit._source[dateField];
        }).map((hit) => {
            let obj = {
                url: urlPrefix + hit._source[urlField],
                displayUrl: urlPrefix + hit._source[urlField],
                title: hit._source["title"] || "",
                description: (hit._source[textField] || "").slice(0,500),
            };
            return obj;
        });
        callback(null, res);
        return res;
        try {
            let url = 'http://' + self._username + ':' +
                self._password + '@' +
                self._host + ':' +
                self._port +
                '/' + index + '/_search?q=' + query + '&from=0&size=' + limit;

            if (log.debug())
                log.debug('Fetching URL: ' + url);
            console.log("url request", url);
            let resp = syncreq('GET', url);

            if (resp.statusCode !== 200) {
                log.warn('Received non-OK status code: %d', resp.statusCode);
                log.warn('Response:\n%s', resp.body.toString());
            }
            let res = self._parseResp(resp.getBody());
            callback(null, res);
            return res;
        } catch (e) {
            log.error(e, 'Failed to execute query, returning NULL!');
            callback(e, null);
            return [];
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

            let itemId = item._source.id;
            let title = item._source.title;
            let description = item._source.abstract;
            let itemUrl = 'https://academic.microsoft.com/paper/' + itemId;

            if (description == null) {
                description = title;
            }

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

        return result;
    }
}

module.exports = exports = function (opts) {
    let source = new MedlineDataSource(opts);
    return async function (query, limit, callback) {
        return await source.fetchData(query, limit, callback);
    }
}
