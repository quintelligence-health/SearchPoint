/*!
 * express-session
 * Copyright(c) 2010 Sencha Inc.
 * Copyright(c) 2011 TJ Holowaychuk
 * Copyright(c) 2015 Douglas Christopher Wilson
 * MIT Licensed
 */

/**
 * Module dependencies.
 * @private
 */

var BaseStore = require('express-session').MemoryStore;
var util = require('util')

/**
 * Shim setImmediate for node.js < 0.10
 * @private
 */

/* istanbul ignore next */
var defer = typeof setImmediate === 'function' ? setImmediate :
                                             function(fn){ process.nextTick(fn.bind.apply(fn, arguments)) }

/**
 * Module exports.
 */

module.exports = MemoryStore

function cleanup(store) {
    var sessions = store.sessions;
    var now = Date.now();

    var cleanupSession = function (sessionId) {
        var session = sessions[sessionId];
        var expires = typeof session.cookie.expires === 'string' ? new Date(session.cookie.expires) : session.cookie.expires;

        if (expires && expires <= now) {
            store.destroy(sessionId, function () {
            });
        }
    }

    for (var sessionId in sessions) {
        cleanupSession(sessionId);
    }
}

/**
 * A session store in memory.
 * @public
 */
function MemoryStore() {
    BaseStore.call(this)
    this.sessions = Object.create(null)

    // cleanup the session store every 10 seconds
    var that = this;
    setInterval(function () {
        cleanup(that);
    }, 30*1000);
}

/**
 * Inherit from Store.
 */
util.inherits(MemoryStore, BaseStore)

/**
 * Get all active sessions.
 *
 * @param {function} callback
 * @public
 */

MemoryStore.prototype.all = function all(callback) {
    var sessionIds = Object.keys(this.sessions)
    var sessions = Object.create(null)

    for (var i = 0; i < sessionIds.length; i++) {
        var sessionId = sessionIds[i]
        var session = getSession.call(this, sessionId)

        if (session) {
            sessions[sessionId] = session;
        }
    }

    callback && defer(callback, null, sessions)     // jshint ignore:line
}

/**
 * Clear all sessions.
 *
 * @param {function} callback
 * @public
 */

MemoryStore.prototype.clear = function clear(callback) {
    this.sessions = Object.create(null)
    callback && defer(callback) // jshint ignore:line
}

/**
 * Destroy the session associated with the given session ID.
 *
 * @param {string} sessionId
 * @public
 */

MemoryStore.prototype.destroy = function destroy(sessionId, callback) {
    this.emit('preDestroy', sessionId, this.sessions[sessionId]);
    delete this.sessions[sessionId];
    this.emit('postDestroy', sessionId);
    callback && defer(callback) // jshint ignore:line
}

/**
 * Fetch session by the given session ID.
 *
 * @param {string} sessionId
 * @param {function} callback
 * @public
 */

MemoryStore.prototype.get = function get(sessionId, callback) {
    defer(callback, null, getSession.call(this, sessionId))
}

/**
 * Commit the given session associated with the given sessionId to the store.
 *
 * @param {string} sessionId
 * @param {object} session
 * @param {function} callback
 * @public
 */

/**
 * Get number of active sessions.
 *
 * @param {function} callback
 * @public
 */

MemoryStore.prototype.length = function length(callback) {
    this.all(function (err, sessions) {
        if (err) return callback(err)
        callback(null, Object.keys(sessions).length)
    })
}

MemoryStore.prototype.set = function set(sessionId, session, callback) {
    this.sessions[sessionId] = session;
    callback && defer(callback)     // jshint ignore:line
}

/**
 * Touch the given session object associated with the given session ID.
 *
 * @param {string} sessionId
 * @param {object} session
 * @param {function} callback
 * @public
 */

MemoryStore.prototype.touch = function touch(sessionId, session, callback) {
    var currentSession = getSession.call(this, sessionId)

    if (currentSession) {
        // update expiration
        currentSession.cookie = session.cookie
        this.sessions[sessionId] = currentSession
    }

    callback && defer(callback)     // jshint ignore:line
}

MemoryStore.prototype.regenerate = function (req, fn) {
    var self = this;
    this.destroy(req.sessionID, function(err){
        self.generate(req);
        fn(err);
    });
}

/**
 * Get session from the store.
 * @private
 */
function getSession(sessionId) {
    return this.sessions[sessionId];
}
