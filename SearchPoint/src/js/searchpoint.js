let decorators = require('./utils/decorators');
let sp = require('bindings')('sp.node');

class SearchPoint extends sp.SearchPoint {

    constructor(opts) {
        super(opts.settings);
    }

    rerank(state, pos, page) {
        return this._rerank(state, pos, page);
    }

    getWidget(state, callback) {
        let self = this;

        let pos = {
            x: 0.5,
            y: 0.5
        }

        let items = self._rerank(state, pos, 0);
        let clusters = state.getClusters();
        let totalItems = state.totalItems;

        callback(undefined, {
            items: items,
            clusters: clusters,
            totalItems: totalItems
        });
    }

    fetchKeywords(state, pos) {
        return super.fetchKeywords(state, pos.x, pos.y);
    }

    _rerank(state, pos, page) {
        let indexes = super.rerank(state, pos.x, pos.y, page);
        let items = state.getByIndexes(indexes);
        return items;
    }
}

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

class SearchPointStore extends SearchPoint {

    constructor(opts) {
        super(opts);

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

            self.getWidget(state, callback);
        });
    }

    rerank(userId, pos, page) {
        let self = this;
        let state = self._getCachedState(userId);
        return super.rerank(state, pos, page);
    }

    fetchKeywords(userId, pos) {
        let self = this;
        let state = self._getCachedState(userId);
        return super.fetchKeywords(state, pos);
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

        if (resultH.has(userId)) {
            resultH.delete(userId);
            if (log.debug())
                log.debug('userId `%s` removed!', userId);
        } else {
            if (log.debug())
                log.debug('cannot delete userId `%s` it does not exist!', userId);
        }

        if (log.debug())
            log.debug('total number of users: %d', resultH.size);
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
        for (let userId of keys) {
            let wrapper = self._resultH.get(userId);
            if (now - wrapper.timestamp > self._timeout) {
                log.info('removing userId `%s`', userId);
                self.removeData(userId);
            }
        }
    }
}

//===================================
// DECORATIONS
//===================================

// SearchPoint
// decorators.ExceptionWrapper.wrapClassAsyncFunction(SearchPoint, 'rerank');
decorators.ExceptionWrapper.wrapClassAsyncFunction(SearchPoint, 'getWidget');
// decorators.ExceptionWrapper.wrapClassAsyncFunction(SearchPoint, 'fetchKeywords');

// SearchPointStore
decorators.ExceptionWrapper.wrapClassAsyncFunction(SearchPointStore, 'createClusters');
decorators.ExceptionWrapper.wrapClassAsyncFunction(SearchPointStore, 'shutdown');
// decorators.wrapAsync(Seae)

//===================================
// EXPORTS
//===================================

exports.SearchPointState = sp.SearchPointState;
exports.SearchPointBase = sp.SearchPoint;
exports.SearchPoint = SearchPoint;
exports.SearchPointStore = SearchPointStore;
