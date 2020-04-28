let express = require('express');
let session = require('express-session');
var randomstring = require("randomstring");

let path = require('path');
let async = require('async');

let MemoryStore = require('./util/memorystore');
let utils = require('./util/utils');

let API_PATH = '/api';

let QUERY_PARAM = 'q';
let QUERY_ID_PARAM = 'qid';
let CLUST_KEY_PARAM = 'c';
let LIMIT_PARAM = 'n';
let PAGE_PARAM = 'p';
let COORD_X_PARAM = 'x';
let COORD_Y_PARAM = 'y';
let POSITIONS_PARAM = 'pos';

let DEFAULT_LIMIT = 200;

module.exports = exports = function (opts) {
    let log = opts.log;
    let port = opts.port;
    let timeout = opts.sessionTimeout;
    let fetchFun = opts.dataSource;
    let sp = opts.sp;

    let executorH = new Map();

    //==================================================================
    // SERVER
    //==================================================================

    let app = express();
    let server;


    let store = new MemoryStore();
    store.on('preDestroy', function (sessionId) {
        if (log.debug())
            log.debug('will remove session data');

        sp.removeData(sessionId);
        if (executorH.has(sessionId)) { executorH.delete(sessionId); }
    })

    app.use(session({
        store: store,
        secret: randomstring.generate({
            length: 12,
            charset: 'alphabetic'
        }),
        cookie: {
            maxAge: timeout
        },
        resave: false,
        saveUninitialized: true
    }))

    function initRestApi() {
        app.get(API_PATH + '/query', function (req, res) {
            try {
                let sessionId = req.sessionID;
                let query = req.query[QUERY_PARAM];//encodeURI(req.query[QUERY_PARAM]);
                let clustKey = req.query[CLUST_KEY_PARAM];
                let limit = LIMIT_PARAM in req.query ? parseInt(req.query[LIMIT_PARAM]) : DEFAULT_LIMIT;

                if (isNaN(limit)) limit = DEFAULT_LIMIT;

                if (log.debug())
                    log.debug('Processing query: query: %s, clust: %s, limit: %d ...', query, clustKey, limit);

                if (!executorH.has(sessionId)) { executorH.set(sessionId, utils.executeLastExecutor()); }
                let execute = executorH.get(sessionId);

                let onCanceled = function () {
                    log.debug('Request cancelled in favor of newer request!');
                    res.status(204);
                    res.end();
                }

                let fetchAndCluster = function (done) {
                    let items = null;
                    let tasks = [
                        function (xcb) {
                            fetchFun(query, limit, function (e, _items) {
                                if (e != null) return xcb(e);
                                items = _items;
                                xcb();
                            });
                        },
                        function (xcb) {
                            sp.createClusters(sessionId, clustKey, items, xcb);
                        }
                    ]

                    async.series(tasks, function (e, result) {
                        if (e != null) {
                            log.error(e, 'Failed to execute query!');
                            res.status(500);
                            res.end();
                            done();
                            return;
                        }

                        if (log.debug())
                            log.debug('Done!');

                        res.send(result[1]);
                        res.end();
                        done();
                    })
                }

                execute(fetchAndCluster, onCanceled);
            } catch (e) {
                log.error(e, 'Failed to execute query!');
                res.status(500);   // internal server error
            }
        });

        app.get(API_PATH + '/rank', function (req, res) {
            try {
                let sessionId = req.sessionID;
                let queryId = req.query[QUERY_ID_PARAM];
                let page = parseInt(req.query[PAGE_PARAM]);
                let pos = req.query[POSITIONS_PARAM][0];

                if (log.trace())
                    log.trace('Ranking: queryId: %s, page: %d, pos: %s', queryId, page, JSON.stringify(pos));

                if (page < 0) throw new Error('Invalid page: ' + page);

                pos.x = parseFloat(pos.x);  // TODO fix this on the client
                pos.y = parseFloat(pos.y);

                let reranked = sp.rerank(sessionId, pos, page);
                res.send(reranked);
                res.end();
            } catch (e) {
                log.error(e, 'Failed to query rank!');
                res.status(500);   // internal server error
                res.end();
            }
        });

        app.get(API_PATH + '/keywords', function (req, res) {
            try {
                let sessionId = req.sessionID;
                let x = parseFloat(req.query[COORD_X_PARAM]);
                let y = parseFloat(req.query[COORD_Y_PARAM]);

                if (log.trace())
                    log.trace('Fetching keywords queryId: %s, x: %d, y: %d ...', x, y);

                let pos = {
                    x: x,
                    y: y
                }

                let keywords = sp.fetchKeywords(sessionId, pos);
                res.send(keywords);
                res.end();
            } catch (e) {
                log.error(e, 'Failed to query keywords!');
                res.status(500);   // internal server error
                res.end();
            }
        });
    }

    //serve static directories on the UI path
    app.use('/', express.static(path.join(__dirname, '../ui')));

    var express = require('express');
    //var app = express();
    
    //setting middleware

    app.use(express.static(_dirname + 'public')); //serving resources from public folder
    
    var server = app.listen(5000);

    initRestApi();

    // start server
    server = app.listen(port);

    log.info('================================================');
    log.info('Server running at http://localhost:%d', port);
    log.info('================================================');
}
