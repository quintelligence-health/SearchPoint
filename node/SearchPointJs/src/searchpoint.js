let path = require('path');

let utils = require('./util/utils');
let sp = require(path.join(__dirname, '../../SearchPointCpp/'));


class ResultWrapper {

    constructor(opts) {
        let self = this;

        if (opts.state == null) throw new Error('Parameter `state` missing!');

        self._state = opts.state;
        self._timestamp = Date.now();
    }

    get state() {
        return this._state;
    }

    get timestamp() {
        return this._timestamp;
    }

    touch() {
        this._timestamp = Date.now();
    }
}

class SearchPoint extends sp.SearchPoint {

    constructor(opts) {
        super(opts.settings);

        let self = this;

        if (opts.timeout == null) throw new Error('Parameter `timeout` missing!');
        if (opts.log == null) throw new Error('Parameter `log` missing!');

        if (opts.cleanup == null) { opts.cleanup = 'auto'; }
        if (opts.cleanupInterval == null) { opts.cleanupInterval = 1000*60*60; }

        self._log = opts.log;
        self._resultH = new Map();
        self._executorH = new Map();

        // cleanup parameters
        self._cleanup = opts.cleanup;
        self._timeout = opts.timeout;
        self._cleanupInterval = opts.cleanupInterval;
        self._intervalId = null;

        if (self._cleanup == 'auto') {
            self._intervalId = setInterval(function () {
                self._checkExpired();
            }, self._cleanupInterval);
        }
    }

    createClusters(userId, widgetKey, items, callback) {
        let self = this;
        let log = self._log;

        super.createClusters(widgetKey, items, function (e, state) {
            if (e != null) return callback(e);

            if (log.debug())
                log.debug('Adding user id `%s`', userId);

            self._resultH.set(userId, new ResultWrapper({
                state: state
            }))

            let pos = {
                x: 0.5,
                y: 0.5
            }

            let items = self.rerank(userId, pos, 0);
            let clusters = state.getClusters();
            let totalItems = state.totalItems;

            callback(undefined, {
                items: items,
                clusters: clusters,
                totalItems: totalItems
            });
        });
    }

    rerank(userId, pos, page) {
        let self = this;
        let state = self._getCachedState(userId);
        let indexes = super.rerank(state, pos.x, pos.y, page);
        let items = state.getByIndexes(indexes);
        return items;
    }

    fetchKeywords(userId, pos) {
        let self = this;
        let state = self._getCachedState(userId);
        return super.fetchKeywords(state, pos.x, pos.y);
    }

    shutdown(callback) {
        let self = this;
        if (self._intervalId != null) {
            clearInterval(self._intervalId);
        }
        callback();
    }

    removeData(userId) {
        let self = this;
        let log = self._log;

        let resultH = self._resultH;

        if (self._cleanup != 'manual') throw new Error('Invalid cleanup type!');
        if (!resultH.has(userId)) throw new Error('User id `' + userId + '` is missing!');

        resultH.delete(userId);
        log.info('userId `%s` removed, total number of users: %d!', userId, resultH.size);
    }

    _getCachedState(userId) {
        if (!this._resultH.has(userId)) throw new Error('Session expired for user `' + userId + '`!');
        let wrapper = this._resultH.get(userId);
        if (wrapper == null) throw new Error('WTF!? Wrapper undefined!');
        wrapper.touch();
        let state = wrapper.state;
        if (state == null) throw new Error('WTF!? ResultWrapper.state is undefined!');
        return state;
    }

    _checkExpired() {
        let self = this;
        let log = self._log;
        let now = Date.now();
        let keys = Array.from(self._resultH.keys());
        for (let key of keys) {
            let wrapper = self._resultH.get(key);
            if (now - wrapper.timestamp > self._timeout) {
                log.info('removing userId `%s`', key);
                self._resultH.delete(key);
            }
        }
    }
}

exports.SearchPoint = SearchPoint;
