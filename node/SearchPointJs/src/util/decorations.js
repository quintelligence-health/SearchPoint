let utils = require('./utils');
let sp = require('../searchpoint');

exports.decorate = function (opts) {
    let log = opts.log;

    let logWrapper = new utils.LogWrapper({
        log: log
    })

    logWrapper.wrapClassAsyncFunction(sp.SearchPoint, 'createClusters');
    logWrapper.wrapClassFunction(sp.SearchPoint, 'rerank');
    logWrapper.wrapClassFunction(sp.SearchPoint, 'fetchKeywords');
    logWrapper.wrapClassAsyncFunction(sp.SearchPoint, 'shutdown');
    logWrapper.wrapClassFunction(sp.SearchPoint, 'removeData');
    logWrapper.wrapClassFunction(sp.SearchPoint, '_getCachedState');
    logWrapper.wrapClassFunction(sp.SearchPoint, '_checkExpired');
}
