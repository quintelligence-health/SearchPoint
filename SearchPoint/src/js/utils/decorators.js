class ExceptionWrapper {

    static wrapAsync(fn) {
        return function () {
            // let args = arguments;
            let callback = arguments[arguments.length-1];

            try {
                return fn.apply(this, arguments);
            } catch (e) {
                callback(e);
            }
        };
    }

    static wrapClassAsyncFunction(clazz, methodName) {
        const method = clazz.prototype[methodName];

        if (method == null) throw new Error('Method ' + methodName + ' missing!');

        Object.defineProperty(clazz.prototype, methodName, {
            value: ExceptionWrapper.wrapAsync(method),
            writable: true
        });
    }
}

exports.ExceptionWrapper = ExceptionWrapper;
