let syncreq = require('sync-request');

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
      let host_and_port;
      if (self._host !== "localhost") {
        host_and_port=self._host;
      }else{
        host_and_port= self._host + ':' + self._port;
      }

      let url = 'http://' + self._username + ':' +
        self._password + '@' +
        host_and_port+
        '/mag-pipeline-naiades/_search?q=' + query + '&from=0&size=' + limit;

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
  return function (query, limit, callback) {
    return source.fetchData(query, limit, callback);
  }
}
