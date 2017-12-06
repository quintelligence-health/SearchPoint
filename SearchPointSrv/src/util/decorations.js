let utils = require('./utils');
let sp = require('searchpoint');

exports.decorate = function (opts) {
    let log = opts.log;

    let logWrapper = new utils.LogWrapper({
        log: log
    })

    logWrapper.wrapClassAsyncFunction(sp.SearchPointStore, 'createClusters');
    logWrapper.wrapClassFunction(sp.SearchPointStore, 'rerank');
    logWrapper.wrapClassFunction(sp.SearchPointStore, 'fetchKeywords');
    logWrapper.wrapClassAsyncFunction(sp.SearchPointStore, 'shutdown');
    logWrapper.wrapClassFunction(sp.SearchPointStore, 'removeData');
    logWrapper.wrapClassFunction(sp.SearchPointStore, '_getCachedState');
    logWrapper.wrapClassFunction(sp.SearchPointStore, '_checkExpired');
}
