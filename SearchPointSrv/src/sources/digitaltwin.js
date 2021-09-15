let syncreq = require('sync-request');
let {Client}=require('@elastic/elasticsearch');
const fs=require('fs');
class MedlineDataSource {
    constructor(opts) {
        let self = this;

        let config = opts.config;
        let content=fs.readFileSync('config/config-digital-twin.json');
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
        let resource=query.resource;
        let entity=query.entity;
        let entityType=query.entityType;
        let corpusInfo = this.config.resourceInfo.corpusInfo[resource];
        let dateRangeInfo = this.config.resourceInfo.dateRange[resource];
        let range = [dateRangeInfo[0], dateRangeInfo[1]];
        if (query.month) {
            let st = query.month + "-01";
            let en = query.month + "-31";
            range = [st, en];
            if (range[0] < st) {
                range[0] = st;
            }
            if (range[1] > en) {
                range[1] = en;
            }
        }
        let fieldKeyword = {};
        fieldKeyword[entityType + ".keyword"] = {
            "value": entity
        }
        const result = await this.client.search({
            index: corpusInfo.indices[entityType],
            body: {
                "size": 20,
                "query": {
                    "bool": {
                        "filter": {
                            "bool": {
                                "must": [
                                    {
                                        "range": {
                                            "Date": {
                                                "gte": range[0],
                                                "lte": range[1]
                                            }
                                        }
                                    },
                                    {
                                        "term": fieldKeyword
                                    }
                                ]
                            }
                        }
                    }
                }
            }
        });

        let res = result.body.hits.hits.filter((hit) => {
            return hit._source.Date;
        }).map((hit) => {
            let obj = {
                url: hit._source[corpusInfo.idCol],
                displayUrl: hit._source[corpusInfo.idCol],
                title: hit._source[corpusInfo.titleCol] || "",
                description: hit._source[corpusInfo.textCol] || "",
            };
            return obj;
        });
        callback(null,res);
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
