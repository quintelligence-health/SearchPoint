let https = require('https');

/**
 * A task executor which always executes only the last task in the queue (if
 * tasks are piling up, the middle ones are forgotten).
 */
exports.executeLastExecutor = function () {
    let currTask = null;
    let pendingTask = null;
    let pendingCancelCb = null

    let executeCurrTask = function () {
        if (currTask == null) return;
        currTask(function () {
            // finished the current task, now execute whichever task is pending
            currTask = pendingTask;
            pendingTask = null;
            executeCurrTask();
        })
    }

    return function execute(task, cancelCb) {
        if (currTask == null) {
            currTask = task;
            executeCurrTask();
        } else {
            if (pendingTask != null) {
                pendingCancelCb();
            }
            pendingTask = task;
            pendingCancelCb = cancelCb != null ? cancelCb : function () {};
        }
    }
}

//=======================================
// HTTP
//=======================================

function httpsRequest(opts, content, cb) {
    if (arguments.length == 2) {
        // callback is in content
        return httpsRequest(opts, null, content);
    }

    // wrap the callback so that it is forced to only run once
    let callback = function () {
        callback = function () {};
        cb.apply(cb, arguments);
    }

    try {
        let req = https.request(opts, function (res) {
            let status = res.statusCode;

            let received = '';
            res.on('data', function (chunk) {
                received += chunk;
            })
            res.on('end', function () {
                if (200 <= status && status < 300) {
                    callback(undefined, received.length != 0 ? received : undefined);
                } else {
                    callback(new Error('Request failed, status code: ' + status), received);
                }
            })
        })
        req.on('socket', function (socket) {
            socket.setTimeout(1000*60*5);   // set timeout to 5 minutes
            socket.on('timeout', function () {
                req.abort();
            })
        })
        req.on('error', function (e) {
            callback(e);
        });

        if (content != null) {
            req.write(JSON.stringify(content));
        }

        req.end();
    } catch (e) {
        callback(e);
    }
}

exports.httpsGetJson = function (host, path, params, headers, callback) {
    if (arguments.length == 3) {
        // callback is in params
        return exports.httpsGetJson(host, path, {}, {}, params);
    }
    if (arguments.length == 4) {
        // callback is in headers
        return exports.httpsGetJson(host, path, params, {}, headers);
    }

    let paramKeys = Object.keys(params);

    if (paramKeys.length > 0) { path += '?' + paramKeys[0] + '=' + params[paramKeys[0]]; }
    for (let keyN = 1; keyN < paramKeys.length; ++keyN) {
        let key = paramKeys[keyN];
        let val = params[key];
        path += '&' + key + '=' + val;
    }

    let opts = {
        hostname: host,
        path: path,
        method: 'GET',
        headers: headers
    }

    httpsRequest(opts, function (e, result) {
        if (e != null) return callback(e);
        if (result != null && typeof result == 'string') { result = JSON.parse(result); }
        callback(undefined, result);
    });
}

exports.httpsPostJson = function (host, path, content, callback) {
    let opts = {
        hostname: host,
        path: path,
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        }
    }

    httpsRequest(opts, content, callback);
}

//=======================================
// Decorations
//=======================================

function wrap(before, fn, after) {
    return function () {
        before();
        let result = fn.apply(this, arguments);
        after();
        return result;
    }
}

function wrapAsync(before, fn, after) {
    return function () {
        let args = arguments;

        let callback = args[args.length-1];
        args[args.length-1] = function () {
            after();
            callback.apply(this, arguments);
        }

        before();
        return fn.apply(this, args);
    }
}

function wrapClassFunction(clazz, methodName, before, after) {
    const method = clazz.prototype[methodName];
    const section = clazz.name + '.' + methodName;

    if (method == null) throw new Error('Method ' + section + ' missing!');

    Object.defineProperty(clazz.prototype, methodName, {
        value: wrap(before, method, after),
        writable: true
    })
}

function wrapClassAsyncFunction(clazz, methodName, before, after) {
    const method = clazz.prototype[methodName];
    const section = clazz.name + '.' + methodName;

    if (method == null) throw new Error('Method ' + section + ' missing!');

    Object.defineProperty(clazz.prototype, methodName, {
        value: wrapAsync(before, method, after),
        writable: true
    })
}

exports.LogWrapper = class LogWrapper {

    constructor(opts) {
        let self = this;

        if (opts.log == null) throw new Error('Paramter `log` missing!');

        self._log = opts.log;
    }

    wrapClassFunction(clazz, methodName) {
        let self = this;
        let log = self._log;

        let before = function () {
            if (log.debug())
                log.debug('entering %s.%s', clazz.name, methodName);
        }
        let after = function () {
            if (log.debug())
                log.debug('%s.%s finished', clazz.name, methodName);
        }

        wrapClassFunction(clazz, methodName, before, after);
    }

    wrapClassAsyncFunction(clazz, methodName) {
        let self = this;
        let log = self._log;

        let before = function () {
            if (log.debug())
                log.debug('entering %s.%s', clazz.name, methodName);
        }
        let after = function () {
            if (log.debug())
                log.debug('%s.%s finished', clazz.name, methodName);
        }

        wrapClassAsyncFunction(clazz, methodName, before, after);
    }
}
