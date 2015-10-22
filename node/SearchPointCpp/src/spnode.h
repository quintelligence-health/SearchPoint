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

/**
 * Search point javascript instance.
 *
 * @constructor
 * @param {Array<String>} apiKeys - a list of BING API keys used to fetch results
 * @param {String} unicodePath - path to the unicode definition file
 * @param {String} dmozPath - path to the DMOZ data file
 */
class TNodeJsSearchPoint: public node::ObjectWrap, public TSpDataSource {
	friend class TNodeJsUtil;
public:
	static void Init(v8::Handle<v8::Object> Exports);
	static const TStr GetClassId() { return "SearchPoint"; };
private:
	const static TStr DEFAULT_CLUST;
	const static TStr JS_DATA_SOURCE_TYPE;
	const static int PER_PAGE;

	TSpSearchPointImpl* SearchPoint;
	v8::Persistent<v8::Function> QueryCallback;	// used if the data source is a function

	TNodeJsSearchPoint(const TClustUtilH& ClustUtilH, const TStr& DefaultClust, const int& PerPage, v8::Local<v8::Function> DataSourceCall);
	TNodeJsSearchPoint(TSpSearchPointImpl* SearchPoint);
	~TNodeJsSearchPoint();

	static TNodeJsSearchPoint* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);

public:
	/**
	 * Processes the given query and returns the results as a JSON object.
	 *
	 * @param {String} query - the search query
	 * @param {String} clust - the clustering key
	 * @param {Number} n - the number of results that will be returned
	 * @returns {Object} - JSON representation of the results
	 */
	JsDeclareFunction(processQuery);

	/**
	 * Returns re-ranked results based on the passed position.
	 *
	 * @param {Number} x - the x coordinate
	 * @param {Number} y - the y coordinate
	 * @param {Number} page - the page which will be returned
	 * @param {String} queryId - identifier of the query
	 */
	JsDeclareFunction(rankByPos);
	JsDeclareFunction(fetchKeywords);
	JsDeclareFunction(getQueryId);

	void ExecuteQuery(PSpResult& PResult, const int NResults=TSpDataSource::DEFAULT_LIMIT);

private:
	void Clr();
};

//class TNodeJsDataSource : public node::ObjectWrap, public TSpDataSource {
//	friend class TNodeJsUtil;
//public:
//	static void Init(v8::Handle<v8::Object> Exports);
//	static const TStr ClassId;
//
//private:
//
//	v8::Persistent<v8::Function> QueryCallback;
//
//	TNodeJsDataSource(v8::Handle<v8::Function>& QueryCallback);
//	~TNodeJsDataSource();
//
//	static TNodeJsDataSource* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args);
//
//public:
//	void ExecuteQuery(PSpResult& PResult, const int NResults=TSpDataSource::DEFAULT_LIMIT);
//
//	JsDeclareProperty(type);	// TODO
//};


#endif /* SRC_SPNODE_H_ */
