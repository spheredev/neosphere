/**
 *  miniRT events CommonJS module
 *  (c) 2015-2016 Fat Cerberus
**/

'use strict';
module.exports =
{
    Delegate: Delegate,
};

const link = require('link');

function Delegate()
{
    this.add    = add;
    this.invoke = invoke;
    this.remove = remove;

    var invokeList = [];

    function haveHandler(handler, thisObj)
    {
        return link(invokeList).contains(function(v) {
            return v.handler == handler && v.thisObj == thisObj;
        });
    }

    // Delegate:add()
    // registers a handler to the delegate.
    // arguments:
    //     handler: a function to be called when the delegate is invoked.
    //     thisObj: optional.  an object to use for the `this` binding of
    //              `handler`. (default: undefined)
    function add(handler, thisObj)
    {
        if (haveHandler(handler, thisObj))
            Abort("Delegate:add(): attempt to add duplicate handler", -1);
        invokeList.push({ thisObj: thisObj, handler: handler });
    }

    // Delegate:invoke()
    // calls all methods in this delegate's invocation list.
    // returns:
    //     the return value of the last handler called.
    function invoke(/*...*/)
    {
        var lastResult = undefined;
        var invokeArgs = arguments;
        link(invokeList).each(function(v) {
            lastResult = v.handler.apply(v.thisObj, invokeArgs);
        });

        // use the return value of the last handler called
        return lastResult;
    }

    // Delegate:remove()
    // remove a handler that was previously registered with add().
    // remarks:
    //     takes the same arguments, with the same semantics, as Delegate:add().
    //     Delegate:add() must already have been called with the same arguments.
    function remove(handler, thisObj)
    {
        if (!haveHandler(handler, thisObj))
            Abort("Delegate:remove(): no such handler is registered", -1);
        link(invokeList)
            .where(function(v) { return v.handler == handler; })
            .where(function(v) { return v.thisObj == thisObj; })
            .remove();
    }
}
