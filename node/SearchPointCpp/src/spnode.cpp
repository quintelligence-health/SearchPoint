#include "spnode.h"

using namespace TSp;

v8::Persistent<v8::Function> TNodeJsSpResult::Constructor;

void TNodeJsSpResult::Init(v8::Handle<v8::Object> Exports) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsSpResult>);
    // child will have the same properties and methods, but a different callback: _NewCpp
    v8::Local<v8::FunctionTemplate> child = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewCpp<TNodeJsSpResult>);
    child->Inherit(tpl);

    child->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));
    // ObjectWrap uses the first internal field to store the wrapped pointer
    child->InstanceTemplate()->SetInternalFieldCount(1);

    tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));
    // ObjectWrap uses the first internal field to store the wrapped pointer.
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Add all methods, getters and setters here.
    NODE_SET_PROTOTYPE_METHOD(tpl, "getClusters", _getClusters);
    NODE_SET_PROTOTYPE_METHOD(tpl, "getByIndexes", _getByIndexes);

    tpl->InstanceTemplate()->SetAccessor(v8::String::NewFromUtf8(Isolate, "totalItems"), _totalItems);

    // This has to be last, otherwise the properties won't show up on the object in JavaScript	
    // Constructor is used when creating the object from C++
    Constructor.Reset(Isolate, child->GetFunction());
    // we need to export the class for calling using "new SearchPointResult(...)"
    Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TNodeJsSpResult::TNodeJsSpResult(const TStr& _WidgetKey, const PJsonVal& JsonItemV):
        ItemV(JsonItemV->GetArrVals(), 0),
        WidgetKey(_WidgetKey) {

    const int NItems = JsonItemV->GetArrVals();
    for (int ItemN = 0; ItemN < NItems; ++ItemN) {
        const PJsonVal& ItemJson = JsonItemV->GetArrVal(ItemN);

        EAssertR(ItemJson->GetObjKey("title")->IsDef(), "Title is not defined!");
        EAssertR(ItemJson->GetObjKey("description")->IsDef(), "Item field description is not defined!");
        EAssertR(ItemJson->GetObjKey("url")->IsDef(), "Item field url is not defined!");
        EAssertR(ItemJson->GetObjKey("displayUrl")->IsDef(), "Item field displayUrl is not defined!");

        ItemV.Add(TSpItem(
            ItemN,
            ItemJson->GetObjStr("title"),
            ItemJson->GetObjStr("description"),
            ItemJson->GetObjStr("url"),
            ItemJson->GetObjStr("displayUrl")
        ));
    }
}

TNodeJsSpResult* NewFromArgs(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    const TStr WidgetKey = TNodeJsUtil::GetArgStr(Args, 0);
    const PJsonVal JsonItemV = TNodeJsUtil::GetArgJson(Args, 1);
    return new TNodeJsSpResult(WidgetKey, JsonItemV);
}

void TNodeJsSpResult::getClusters(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsSpResult* JsResult = ObjectWrap::Unwrap<TNodeJsSpResult>(Args.Holder());

    PJsonVal JsonClusterV = TJsonVal::NewArr();

    for (int ClusterN = 0; ClusterN < JsResult->ClusterV.Len(); ++ClusterN) {
        const TSpCluster& Cluster = JsResult->ClusterV[ClusterN];

        PJsonVal ClusterJson = TJsonVal::NewObj();

        PJsonVal KwsJson = TJsonVal::NewArr();
        for (int j = 0; j < Cluster.RecIdKwWgtFqTrKdV.Len(); j++) {
            const TStrFltFltTr KwWgtPr = Cluster.RecIdKwWgtFqTrKdV[j].Dat;

            PJsonVal KwJson = TJsonVal::NewObj();

            const PJsonVal TextJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(KwWgtPr.Val1));

            EAssertR(TextJson->IsStr(), "Cluster text JSON is not string!");

            KwJson->AddToObj("text", TextJson);
            KwJson->AddToObj("weight", TJsonVal::NewNum(KwWgtPr.Val2));
            KwJson->AddToObj("fq", TJsonVal::NewNum(KwWgtPr.Val3));

            KwsJson->AddToArr(KwJson);
        }

        //children
        PJsonVal ChildIdxArr = TJsonVal::NewArr();
        for (int j = 0; j < Cluster.ChildIdxV.Len(); j++)
            ChildIdxArr->AddToArr(TJsonVal::NewNum(Cluster.ChildIdxV[j]));

        const PJsonVal TextJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(Cluster.RecIdTopKwKd.Dat));

        EAssertR(TextJson->IsStr(), "Cluster text JSON is not string!");

        ClusterJson->AddToObj("text", TextJson);
        ClusterJson->AddToObj("kwords", KwsJson);
        ClusterJson->AddToObj("x", TJsonVal::NewNum(Cluster.Pos.Val1));
        ClusterJson->AddToObj("y", TJsonVal::NewNum(Cluster.Pos.Val2));
        ClusterJson->AddToObj("size", TJsonVal::NewNum(Cluster.Size));
        ClusterJson->AddToObj("childIdxs", ChildIdxArr);

        JsonClusterV->AddToArr(ClusterJson);
    }

    PJsonVal Result = TJsonVal::NewArr();
    Result->AddToArr(JsonClusterV);

    Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, Result));
}

void TNodeJsSpResult::getByIndexes(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsSpResult* JsResult = ObjectWrap::Unwrap<TNodeJsSpResult>(Args.Holder());

    TIntV IdxV; TNodeJsUtil::GetArgIntV(Args, 0, IdxV);

    PJsonVal JsonItemV = TJsonVal::NewArr();
    for (int ValN = 0; ValN < IdxV.Len(); ++ValN) {
        EAssert(0 <= ValN && ValN < IdxV.Len());
        const int ItemN = IdxV[ValN];
        EAssert(0 <= ItemN && ItemN < JsResult->ItemV.Len());
        const TSpItem& Item = JsResult->ItemV[ItemN];

        PJsonVal ItemJson = TJsonVal::NewObj();
        ItemJson->AddToObj("title", THtmlLx::GetEscapedStr(Item.Title)); // TODO make consistent with input
        ItemJson->AddToObj("description", THtmlLx::GetEscapedStr(Item.Description));
        ItemJson->AddToObj("displayURL", THtmlLx::GetEscapedStr(Item.DisplayUrl));
        ItemJson->AddToObj("URL", THtmlLx::GetEscapedStr(Item.Url));
        ItemJson->AddToObj("rank", ItemN);

        JsonItemV->AddToArr(ItemJson);
    }

    Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, JsonItemV));
}

void TNodeJsSpResult::totalItems(v8::Local<v8::String> Name, const v8::PropertyCallbackInfo<v8::Value>& Info) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsSpResult* JsResult = ObjectWrap::Unwrap<TNodeJsSpResult>(Info.Holder());
    Info.GetReturnValue().Set(v8::Integer::New(Isolate, JsResult->ItemV.Len()));
}

///////////////////////////////////////
/// SearchPoint
const TStr TNodeJsSearchPoint::DEFAULT_CLUST = "kmeans";
const TStr TNodeJsSearchPoint::JS_DATA_SOURCE_TYPE = "func";
const int TNodeJsSearchPoint::PER_PAGE = 10;

void TNodeJsSearchPoint::Init(v8::Handle<v8::Object> Exports) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(Isolate, TNodeJsUtil::_NewJs<TNodeJsSearchPoint>);
    tpl->SetClassName(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()));
    // ObjectWrap uses the first internal field to store the wrapped pointer.
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    // Add all methods, getters and setters here.
    NODE_SET_PROTOTYPE_METHOD(tpl, "createClustersSync", _createClustersSync);
    NODE_SET_PROTOTYPE_METHOD(tpl, "createClusters", _createClusters);
    NODE_SET_PROTOTYPE_METHOD(tpl, "rerank", _rerank);
    NODE_SET_PROTOTYPE_METHOD(tpl, "fetchKeywords", _fetchKeywords);
    /* NODE_SET_PROTOTYPE_METHOD(tpl, "getQueryId", _getQueryId); */

    Exports->Set(v8::String::NewFromUtf8(Isolate, GetClassId().CStr()), tpl->GetFunction());
}

TNodeJsSearchPoint::TNodeJsSearchPoint(const TClustUtilH& ClustUtilH, const TStr& DefaultClust, const int& PerPage, v8::Local<v8::Function> DataSourceCall):
        SearchPoint(nullptr) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    QueryCallback.Reset(Isolate, DataSourceCall);
    SearchPoint = new TSpSearchPointImpl(ClustUtilH, DEFAULT_CLUST, PER_PAGE);
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
        ClustUtilsH.AddDat(DEFAULT_CLUST, new TSpDPMeansClustUtils(Notify));
        ClustUtilsH.AddDat("dmoz", new TSpDmozClustUtils(DmozFilePath));

        return new TNodeJsSearchPoint(new TSpSearchPointImpl(ClustUtilsH, DEFAULT_CLUST, PER_PAGE));
    } catch (const PExcept& Except) {
        TNodeJsUtil::ThrowJsException(Isolate, Except);
        return nullptr;
    }
}

TNodeJsSearchPoint::TCreateClustersTask::TCreateClustersTask(const v8::FunctionCallbackInfo<v8::Value>& Args,
            const bool& _IsAsync) :
        TNodeTask(Args),
        JsResult(nullptr) {


    TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());

    JsResult = new TNodeJsSpResult(
        TNodeJsUtil::GetArgStr(Args, 0),
        TNodeJsUtil::GetArgJson(Args, 1)
    );
    SearchPoint = JsSp->SearchPoint;
}

v8::Handle<v8::Function> TNodeJsSearchPoint::TCreateClustersTask::GetCallback(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    return TNodeJsUtil::GetArgFun(Args, 2);
}

v8::Local<v8::Value> TNodeJsSearchPoint::TCreateClustersTask::WrapResult() {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::EscapableHandleScope HandleScope(Isolate);

    return HandleScope.Escape(TNodeJsUtil::NewInstance(JsResult));
}

void TNodeJsSearchPoint::TCreateClustersTask::Run() {
    try {
        SearchPoint->GenClusters(
            JsResult->WidgetKey,
            JsResult->ItemV,
            JsResult->ClusterV,
            JsResult->ItemClustSimVV,
            JsResult->HasBgClust
        );
    } catch (const PExcept& Except) {
        SetExcept(Except);
    }
}

void TNodeJsSearchPoint::TCreateClustersTask::SetExcept(const PExcept& Except) {
    delete JsResult;
    TNodeTask::SetExcept(Except);
}

void TNodeJsSearchPoint::rerank(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());
    TSpSearchPointImpl* SearchPoint = JsSp->SearchPoint;

    const TNodeJsSpResult* JsSpResult = TNodeJsUtil::GetArgUnwrapObj<TNodeJsSpResult>(Args, 0);
    const double x = TNodeJsUtil::GetArgFlt(Args, 1);
    const double y = TNodeJsUtil::GetArgFlt(Args, 2);
    const int Page = TNodeJsUtil::GetArgInt32(Args, 3);

    TFltPrV PosV;   PosV.Add(TFltPr(x, y));

    TIntV PermuteV;
    SearchPoint->ProcPosPageRq(
        PosV,
        JsSpResult->ClusterV,
        JsSpResult->ItemClustSimVV,
        JsSpResult->HasBgClust,
        Page,
        PermuteV
    );

    v8::Local<v8::Array> JsIdxV = v8::Array::New(Isolate, PermuteV.Len());
    for (int IdxN = 0; IdxN < PermuteV.Len(); ++IdxN) {
        JsIdxV->Set(IdxN, v8::Integer::New(Isolate, PermuteV[IdxN]));
    }

    Args.GetReturnValue().Set(JsIdxV);
}

void TNodeJsSearchPoint::fetchKeywords(const v8::FunctionCallbackInfo<v8::Value>& Args) {
    v8::Isolate* Isolate = v8::Isolate::GetCurrent();
    v8::HandleScope HandleScope(Isolate);

    TNodeJsSearchPoint* JsSp = ObjectWrap::Unwrap<TNodeJsSearchPoint>(Args.Holder());
    TSpSearchPointImpl* SearchPoint = JsSp->SearchPoint;

    const TNodeJsSpResult* JsResult = TNodeJsUtil::GetArgUnwrapObj<TNodeJsSpResult>(Args, 0);
    const double x = TNodeJsUtil::GetArgFlt(Args, 1);
    const double y = TNodeJsUtil::GetArgFlt(Args, 2);

    PJsonVal ResultJson = SearchPoint->ProcessPosKwRq(
        TFltPr(x, y),
        JsResult->ClusterV,
        JsResult->HasBgClust
    );

    Args.GetReturnValue().Set(TNodeJsUtil::ParseJson(Isolate, ResultJson));
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

//////////////////////////////////////////////
// module initialization
void Init(v8::Handle<v8::Object> Exports) {
    TNodeJsSpResult::Init(Exports);
    TNodeJsSearchPoint::Init(Exports);
}

NODE_MODULE(sp, Init);

