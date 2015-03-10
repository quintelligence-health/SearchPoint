#include "spnode.h"

using namespace TSp;

const TStr TNodeJsSearchPoint::ClassId = "SearchPoint";
const TStr TNodeJsSearchPoint::DEFAULT_CLUST = "kmeans";
const TStr TNodeJsSearchPoint::JS_DATA_SOURCE_TYPE = "func";
const int TNodeJsSearchPoint::PER_PAGE = 10;

void TNodeJsSearchPoint::Init(v8::Handle<v8::Object> Exports) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsSearchPoint>);
	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, ClassId.CStr()));
	// ObjectWrap uses the first internal field to store the wrapped pointer.
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Add all methods, getters and setters here.
	NODE_SET_PROTOTYPE_METHOD(tpl, "processQuery", _processQuery);
	NODE_SET_PROTOTYPE_METHOD(tpl, "rankByPos", _rankByPos);
	NODE_SET_PROTOTYPE_METHOD(tpl, "fetchKeywords", _fetchKeywords);
	NODE_SET_PROTOTYPE_METHOD(tpl, "getQueryId", _getQueryId);

	Exports->Set(v8::String::NewFromUtf8(Isolate, ClassId.CStr()), tpl->GetFunction());
}

TNodeJsSearchPoint::TNodeJsSearchPoint(const TClustUtilH& ClustUtilH, const TStr& DefaultClust, const int& PerPage, v8::Local<v8::Function> DataSourceCall):
		SearchPoint(nullptr) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	QueryCallback.Reset(Isolate, DataSourceCall);
	SearchPoint = new TSpSearchPointImpl(ClustUtilH, DEFAULT_CLUST, PER_PAGE, PSpDataSource(this));
}


TNodeJsSearchPoint::TNodeJsSearchPoint(TSpSearchPointImpl* _SearchPoint):
		SearchPoint(_SearchPoint) {}

TNodeJsSearchPoint::~TNodeJsSearchPoint() {
	Clr();
}

TNodeJsSearchPoint* TNodeJsSearchPoint::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	try {
		const PNotify Notify = TStdNotify::New();

		EAssertR(Args.Length() >= 1, "SearchPoint: expects 1 argument!");
		EAssertR(Args[0]->IsObject(), "SearchPoint: argument 0 should be an object!");

		v8::Local<v8::Object> ArgObj = Args[0]->ToObject();
		PJsonVal ArgJson = TNodeJsUtil::GetObjJson(ArgObj, true);

		Notify->OnNotify(TNotifyType::ntInfo, "Parsing arguments ...");

		const TStr UnicodeDefPath = ArgJson->GetObjStr("unicodePath");
		const TStr DmozFilePath = ArgJson->GetObjStr("dmozPath");

		// load the unicode file
		Notify->OnNotify(TNotifyType::ntInfo, "Loading UnicodeDef ...");
		TUnicodeDef::Load(UnicodeDefPath);
		Notify->OnNotify(TNotifyType::ntInfo, "Loaded!");

		TClustUtilH ClustUtilsH;
		ClustUtilsH.AddDat(DEFAULT_CLUST, TSpDPMeansClustUtils::New(Notify));
		ClustUtilsH.AddDat("dmoz", TSpDmozClustUtils::New(DmozFilePath));

		// construct the data source
		v8::Local<v8::Value> DsObj = ArgObj->Get(v8::String::NewFromUtf8(Isolate, "dataSource"));

		if (DsObj->IsFunction()) {
			v8::Local<v8::Function> DsFun = v8::Handle<v8::Function>::Cast(DsObj);
			return new TNodeJsSearchPoint(ClustUtilsH, DEFAULT_CLUST, PER_PAGE, DsFun);
		} else {
			const PJsonVal DataSrcJson = ArgJson->GetObjKey("datasource");
			TStrV ApiKeyV;	DataSrcJson->GetObjStrV("apiKeys", ApiKeyV);
			return new TNodeJsSearchPoint(new TSpSearchPointImpl(ClustUtilsH, DEFAULT_CLUST, PER_PAGE, TSpBingEngine::New(ApiKeyV, Notify)));
		}
	} catch (const PExcept& Except) {
		TNodeJsUtil::ThrowJsException(Isolate, Except);
		return nullptr;
	}
}


void TNodeJsSearchPoint::processQuery(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());
	TSpSearchPointImpl* SearchPoint = JsSp->SearchPoint;

	const TStr QueryStr = TNodeJsUtil::GetArgStr(Args, 0);
	const TStr ClustKey = TNodeJsUtil::GetArgStr(Args, 1);
	const int Limit = TNodeJsUtil::GetArgInt32(Args, 2);

	const PSpResult SpResult = SearchPoint->ExecuteQuery(QueryStr, ClustKey, Limit);
	PJsonVal ResultJson = SearchPoint->GenJSon(SpResult, 0, SearchPoint->PerPage);

	Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, ResultJson));
}

void TNodeJsSearchPoint::rankByPos(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());
	TSpSearchPointImpl* SearchPoint = JsSp->SearchPoint;

	const double x = TNodeJsUtil::GetArgFlt(Args, 0);
	const double y = TNodeJsUtil::GetArgFlt(Args, 1);
	const int Page = TNodeJsUtil::GetArgInt32(Args, 2);
	const TStr QueryId = TNodeJsUtil::GetArgStr(Args, 3);

	TFltPrV PosV;	PosV.Add(TFltPr(x, y));

	PJsonVal ResultJson = SearchPoint->ProcPosPageRq(PosV, Page, QueryId);

	Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, ResultJson));
}

void TNodeJsSearchPoint::fetchKeywords(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());
	TSpSearchPointImpl* SearchPoint = JsSp->SearchPoint;

	const double x = TNodeJsUtil::GetArgFlt(Args, 0);
	const double y = TNodeJsUtil::GetArgFlt(Args, 1);
	const TStr QueryId = TNodeJsUtil::GetArgStr(Args, 2);

	PJsonVal ResultJson = SearchPoint->ProcessPosKwRq(TFltPr(x, y), QueryId);

	Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, ResultJson));
}

void TNodeJsSearchPoint::getQueryId(const v8::FunctionCallbackInfo<v8::Value>& Args) {
	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());
	TSpSearchPointImpl* SearchPoint = JsSp->SearchPoint;

	const TStr Query = TNodeJsUtil::GetArgStr(Args, 0);
	const TStr ClustKey = TNodeJsUtil::GetArgStr(Args, 1);
	const int Limit = TNodeJsUtil::GetArgInt32(Args, 2);

	const TStr QueryId = SearchPoint->GenQueryId(Query, ClustKey, Limit);

	Args.GetReturnValue().Set(v8::String::NewFromUtf8(Isolate, QueryId.CStr()));
}

void TNodeJsSearchPoint::ExecuteQuery(PSpResult& SpResult, const int Limit) {
	if (QueryCallback.IsEmpty()) { return; }

	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
	v8::HandleScope HandleScope(Isolate);

	try {
		const TStr& QueryStr = SpResult->QueryStr;

		// call the callback to get the results
		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, QueryCallback);
		PJsonVal ResultJson = TNodeJsUtil::ExecuteJson(Callback, v8::String::NewFromUtf8(Isolate, QueryStr.CStr())->ToObject(), v8::Integer::New(Isolate, Limit)->ToObject());

		EAssertR(ResultJson->IsArr(), "The result of the callback should be an array!");

		TSpItemV& ItemV = SpResult->ItemV;

		// parse the results
		const int Len = ResultJson->GetArrVals();
		for (int i = 0; i < Len; i++) {
			PJsonVal ItemJson = ResultJson->GetArrVal(i);

			const TStr& Title = ItemJson->GetObjStr("title");
			const TStr& Desc = ItemJson->GetObjStr("description");
			const TStr& Url = ItemJson->GetObjStr("url");
			const TStr& DispUrl = ItemJson->GetObjStr("displayUrl");

			ItemV.Add(TSpItem(i+1, Title, Desc, Url, DispUrl));
		}
	} catch (const PExcept& Except) {
		TNodeJsUtil::ThrowJsException(Isolate, Except);
	}
}

void TNodeJsSearchPoint::Clr() {
	if (!QueryCallback.IsEmpty()) {
		QueryCallback.Reset();
	}

	if (SearchPoint != nullptr) {
		delete SearchPoint;
		SearchPoint = nullptr;
	}
}


//const TStr TNodeJsDataSource::ClassId = "DataSource";
//
//void TNodeJsDataSource::Init(v8::Handle<v8::Object> Exports) {
//	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
//	v8::HandleScope HandleScope(Isolate);
//
//	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsDataSource>);
//	tpl->SetClassName(v8::String::NewFromUtf8(Isolate, ClassId.CStr()));
//	// ObjectWrap uses the first internal field to store the wrapped pointer.
//	tpl->InstanceTemplate()->SetInternalFieldCount(1);
//
//	// Add all methods, getters and setters here.
////	NODE_SET_PROTOTYPE_METHOD(tpl, "processQuery", _processQuery);
////	NODE_SET_PROTOTYPE_METHOD(tpl, "rankByPos", _rankByPos);
////	NODE_SET_PROTOTYPE_METHOD(tpl, "fetchKeywords", _fetchKeywords);
////	NODE_SET_PROTOTYPE_METHOD(tpl, "getQueryId", _getQueryId);
//
//	Exports->Set(v8::String::NewFromUtf8(Isolate, ClassId.CStr()), tpl->GetFunction());
//}
//
//TNodeJsDataSource::TNodeJsDataSource(v8::Handle<v8::Function>& _QueryCallback) {
//	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
//	v8::HandleScope HandleScope(Isolate);
//	QueryCallback.Reset(Isolate, _QueryCallback);
//}
//
//TNodeJsDataSource::~TNodeJsDataSource() {
//	QueryCallback.Reset();
//}
//
//TNodeJsDataSource* TNodeJsDataSource::NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
//	EAssertR(Args.Length() == 1 && Args[0]->IsFunction(), "Argument 1 is not a function!");
//
//	v8::Handle<v8::Function> Callback = v8::Handle<v8::Function>::Cast(Args[0]);
//	return new TNodeJsDataSource(Callback);
//}
//
//void TNodeJsDataSource::ExecuteQuery(PSpResult& Result, const int Limit) {
//	if (QueryCallback.IsEmpty()) { return; }
//
//	v8::Isolate* Isolate = v8::Isolate::GetCurrent();
//	v8::HandleScope HandleScope(Isolate);
//
//	try {
//		// call the callback to get the results
//		v8::Local<v8::Function> Callback = v8::Local<v8::Function>::New(Isolate, QueryCallback);
//		PJsonVal ResultJson = TNodeJsUtil::ExecuteJson(Callback, v8::Integer::New(Isolate, Limit)->ToObject());
//
//		EAssertR(ResultJson->IsArr(), "The result of the callback should be an array!");
//
//		TSpItemV& ItemV = Result->ItemV;
//
//		// parse the results
//		const int Len = ResultJson->GetArrVals();
//		for (int i = 0; i < Len; i++) {
//			PJsonVal ItemJson = ResultJson->GetArrVal(i);
//
//			const TStr& Title = ItemJson->GetObjStr("title");
//			const TStr& Desc = ItemJson->GetObjStr("description");
//			const TStr& Url = ItemJson->GetObjStr("url");
//			const TStr& DispUrl = ItemJson->GetObjStr("displayUrl");
//
//			ItemV.Add(TSpItem(i+1, Title, Desc, Url, DispUrl));
//		}
//	} catch (const PExcept& Except) {
//		TNodeJsUtil::ThrowJsException(Isolate, Except);
//	}
//}

//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
	TNodeJsSearchPoint::Init(Exports);
}

NODE_MODULE(sp, Init);

