/*
 * spnode.h
 *
 *  Created on: Mar 9, 2015
 *      Author: lstopar
 */

#ifndef SRC_SPNODE_H_
#define SRC_SPNODE_H_

#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif

#include <node.h>
#include <node_object_wrap.h>

#include "base.h"
#include "sp.h"
#include "nodeutil.h"

using namespace TSp;

/////////////////////////////
/// SearchPoint result wrapper
class TNodeJsSpResult: public node::ObjectWrap {
    friend class TNodeJsUtil;

    static v8::Persistent<v8::Function> Constructor;
public:
    static void Init(v8::Handle<v8::Object> Exports);
    static const TStr GetClassId() { return "SearchPointState"; };

public:
    TSpItemV ItemV;
    TStr WidgetKey;
    TSpClusterV ClusterV {};
    TFltVV ItemClustSimVV {};
    TBool HasBgClust {false};

    // constructor
    TNodeJsSpResult(const TStr& WidgetKey, const PJsonVal& JsonItemV);
    static TNodeJsSpResult* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

    // SERIALIZATION
    TNodeJsSpResult(TSIn& SIn);
    void Save(TSOut&) const;

    JsDeclareFunction(getClusters);
    JsDeclareFunction(getByIndexes);

    /**
     * Serializes the model into a Node.js Buffer.
     */
    JsDeclareFunction(serialize);

    JsDeclareProperty(totalItems);
};

/**
 * Search point javascript instance.
 *
 * @constructor
 * @param {Array<String>} apiKeys - a list of BING API keys used to fetch results
 * @param {String} unicodePath - path to the unicode definition file
 * @param {String} dmozPath - path to the DMOZ data file
 */
class TNodeJsSearchPoint: public node::ObjectWrap {
	friend class TNodeJsUtil;

    using TClustUtilH = TSpSearchPoint::TClustUtilH;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "SearchPoint"; };
private:
	const static TStr DEFAULT_CLUST;
	const static TStr JS_DATA_SOURCE_TYPE;

	TSpSearchPointImpl* SearchPoint;
	v8::Persistent<v8::Function> QueryCallback;	// used if the data source is a function

	TNodeJsSearchPoint(const TClustUtilH& ClustUtilH, const TStr& DefaultClust, const int& PerPage, v8::Local<v8::Function> DataSourceCall);
	TNodeJsSearchPoint(TSpSearchPointImpl* SearchPoint);
	~TNodeJsSearchPoint();

	static TNodeJsSearchPoint* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

    //////////////////////////////
    /// A task for executing queries
    class TCreateClustersTask : public TNodeTask {
        // search point object
        TSpSearchPoint* SearchPoint;
        // the object which stores the result
        TNodeJsSpResult* JsResult;

    public:
        TCreateClustersTask(const v8::FunctionCallbackInfo<v8::Value>& Args,
                const bool& IsAsync);

        v8::Handle<v8::Function> GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args);
        v8::Local<v8::Value> WrapResult();
        void Run();
        void SetExcept(const PExcept& Except);
    };

public:
	/**
	 * Processes the given query and returns the results as a JSON object.
	 */
	/* JsDeclareFunction(processQuery); */
    JsDeclareSyncAsync(createClustersSync, createClusters, TCreateClustersTask);

	/**
	 * Returns re-ranked results based on the passed position.
	 */
	JsDeclareFunction(rerank);
	JsDeclareFunction(fetchKeywords);

private:
	void Clr();
};


#endif /* SRC_SPNODE_H_ */
