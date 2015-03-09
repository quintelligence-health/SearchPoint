#include "spnode.h"

const TStr TNodeJsSearchPoint::ClassId = "SearchPoint";

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

	Exports->Set(v8::String::NewFromUtf8(Isolate, ClassId.CStr()),
			   tpl->GetFunction());
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

		EAssertR(Args[0]->IsArray(), "Argument 0 should be a vector of API keys!");
		EAssertR(Args[1]->IsString(), "Argument 1 should be a path to the unicode definition file!");
		EAssertR(Args[2]->IsString(), "Argument 2 should be a path to the DMoz data file!");

		Notify->OnNotify(TNotifyType::ntInfo, "Parsing arguments ...");

		const TStr UnicodeDefPath = TNodeJsUtil::GetArgStr(Args, 1);
		const TStr DmozFilePath = TNodeJsUtil::GetArgStr(Args, 2);

		// parse API keys
		TStrV ApiKeyV;
		v8::Handle<v8::Array> Array = v8::Handle<v8::Array>::Cast(Args[0]);

		for (uint i = 0; i < Array->Length(); i++) {
			EAssertR(Array->Get(i)->IsString(), "Array of API keys should only contain strings!");
			v8::Local<v8::Value> ApiKeyVal = Array->Get(i);
			v8::String::Utf8Value Utf8(ApiKeyVal->ToString());

			ApiKeyV.Add(*Utf8);
		}

		// load the unicode file
		Notify->OnNotify(TNotifyType::ntInfo, "Loading UnicodeDef ...");
		TUnicodeDef::Load(UnicodeDefPath);
		Notify->OnNotify(TNotifyType::ntInfo, "Loaded!");

		TStr DefaultClustUtils = "kmeans";
		THash<TStr, PSpClustUtils> ClustUtilsH;

		ClustUtilsH.AddDat(DefaultClustUtils, TSpDPMeansClustUtils::New(Notify));
		ClustUtilsH.AddDat("dmoz", TSpDmozClustUtils::New(DmozFilePath));

		Notify->OnNotify(TNotifyType::ntInfo, "Creating SearchPoint instance ...");

		return new TNodeJsSearchPoint(new TSpSearchPointImpl(ClustUtilsH, DefaultClustUtils, 10, TSpBingEngine::New(ApiKeyV, Notify)));
	} catch (const PExcept& Except) {
		Isolate->ThrowException(v8::Exception::TypeError(
							v8::String::NewFromUtf8(Isolate, TStr("[addon] Exception: " + Except->GetMsgStr()).CStr())));
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

void TNodeJsSearchPoint::Clr() {
	if (SearchPoint != nullptr) {
		delete SearchPoint;
		SearchPoint = nullptr;
	}
}


void Init(v8::Handle<v8::Object> Exports) {
	TNodeJsSearchPoint::Init(Exports);
}

NODE_MODULE(sp, Init);

