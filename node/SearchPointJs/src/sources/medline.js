let httpreq = require('sync-request');

class MedlineDataSource {
    constructor() {

    }

    fetchData(query, limit) {
        // TODO
    }
}

module.exports = exports = function (opts) {
    let source = new MedlineDataSource(opts);
    return function (query, limit) {
        return source.fetchData(query, limit);
    }
}
