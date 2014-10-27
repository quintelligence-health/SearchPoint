#include <jni.h>
#include <stdio.h>
#include <SearchPoint.h>
#include "si_ijs_searchpoint_SearchPoint.h"

const PNotify Notify = TStdNotify::New();
TSpSearchPointImpl* SearchPoint = NULL;

JNIEXPORT void JNICALL Java_si_ijs_searchpoint_SearchPoint_init(JNIEnv * env,
		jobject thisObj, jobjectArray bingApiKeys, jstring dbsPath, jstring dmozPath) {
	setbuf(stdout, NULL);	// eclipse

	try {
		const char* DbsPathChar = env->GetStringUTFChars(dbsPath, JNI_FALSE);
		TStr DbsPath = DbsPathChar;
		env->ReleaseStringUTFChars(dbsPath, DbsPathChar);

		// get the API keys
		int NApiKeys = env->GetArrayLength(bingApiKeys);
		TStrV ApiKeyV(NApiKeys, 0);
		for (int i = 0; i < NApiKeys; i++) {
			jstring ApiKey = (jstring) env->GetObjectArrayElement(bingApiKeys, i);
			const char* ApiKeyChar = env->GetStringUTFChars(ApiKey, JNI_FALSE);

			TStr BingApiKey = ApiKeyChar;

			ApiKeyV.Add(BingApiKey);

			env->ReleaseStringUTFChars(ApiKey, ApiKeyChar);
		}

		Notify->OnNotify(TNotifyType::ntInfo, "Initializing SearchPoint ...");

		if (SearchPoint != NULL) {
			Notify->OnNotify(TNotifyType::ntWarn, "Previous instance of search point detected, destroying previous instance ...");
			delete SearchPoint;
		}

		Notify->OnNotify(TNotifyType::ntInfo, "Loading UnicodeDef ...");
		TUnicodeDef::Load(DbsPath);
		Notify->OnNotify(TNotifyType::ntInfo, "Loaded!");

		TStr DefaultClustUtils = "kmeans";
		THash<TStr, PSpClustUtils> ClustUtilsH;
		ClustUtilsH.AddDat(DefaultClustUtils, TSpDPMeansClustUtils::New(Notify));

		if (dmozPath != NULL) {
			Notify->OnNotify(TNotifyType::ntInfo, "Initializing DMOZ ...");

			const char* DmozPathChar = env->GetStringUTFChars(dmozPath, JNI_FALSE);
			ClustUtilsH.AddDat("dmoz", TSpDmozClustUtils::New(DmozPathChar));

			env->ReleaseStringUTFChars(dmozPath, DmozPathChar);
		}

		Notify->OnNotify(TNotifyType::ntInfo, "Creating SearchPoint instance ...");
		SearchPoint = new TSpSearchPointImpl(ClustUtilsH, DefaultClustUtils, 10, TSpBingEngine::New(ApiKeyV, Notify));
	} catch (...) {
		throw env->ThrowNew(env->FindClass("java.io.IOException"), "Failed to initialize search point!");
	}
}

/*
 * Class:     SearchPointJava
 * Method:    processQuery
 * Signature: (Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_si_ijs_searchpoint_SearchPoint_processQuery(JNIEnv * env, jobject thisObj, jstring query, jstring clustKey, jint limit) {
	const char* QueryChar = env->GetStringUTFChars(query, JNI_FALSE);
	const char* ClustKeyChar = env->GetStringUTFChars(clustKey, JNI_FALSE);

	TStr QueryStr = QueryChar;
	TStr ClustKey = ClustKeyChar;
	TInt Limit = limit;

	env->ReleaseStringUTFChars(query, QueryChar);
	env->ReleaseStringUTFChars(clustKey, ClustKeyChar);

	PSpResult SpResult = SearchPoint->ExecuteQuery(QueryStr, ClustKey, Limit);
	TStr InsertJSon = TJsonVal::GetStrFromVal(SearchPoint->GenJSon(SpResult, 0, SearchPoint->PerPage));

	return env->NewStringUTF(InsertJSon.CStr());
}

/*
 * Class:     SearchPointJava
 * Method:    rankByPos
 * Signature: (DDLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_si_ijs_searchpoint_SearchPoint_rankByPos(JNIEnv * env, jobject thisObj, jdouble x, jdouble y, jint page, jstring queryId) {
	const char* QueryIdChar = env->GetStringUTFChars(queryId, JNI_FALSE);

	TFltPr PosPr(x, y);
	TInt Page = page;
	TStr QueryId = QueryIdChar;

	TFltPrV PosV;	PosV.Add(PosPr);

	env->ReleaseStringUTFChars(queryId, QueryIdChar);

	PJsonVal ResultJson = SearchPoint->ProcPosPageRq(PosV, Page, QueryId);
	TStr Result = TJsonVal::GetStrFromVal(ResultJson);

	return env->NewStringUTF(Result.CStr());
}

/*
 * Class:     SearchPointJava
 * Method:    fetchKeywords
 * Signature: (DDLjava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_si_ijs_searchpoint_SearchPoint_fetchKeywords(JNIEnv * env, jobject thisObj, jdouble x, jdouble y, jstring queryId) {
	const char* QueryIdChar = env->GetStringUTFChars(queryId, JNI_FALSE);

	TFltPr PosPr(x, y);
	TStr QueryId = QueryIdChar;

	env->ReleaseStringUTFChars(queryId, QueryIdChar);

	PJsonVal ResultJson = SearchPoint->ProcessPosKwRq(PosPr, QueryId);
	TStr Result = TJsonVal::GetStrFromVal(ResultJson);

	return env->NewStringUTF(Result.CStr());
}

JNIEXPORT jstring JNICALL Java_si_ijs_searchpoint_SearchPoint_getQueryId(JNIEnv * env, jobject thisObj, jstring query, jstring clustering, jint limit) {
	const char* QueryChar = env->GetStringUTFChars(query, JNI_FALSE);
	const char* ClustChar = env->GetStringUTFChars(clustering, JNI_FALSE);

	TStr QueryStr = QueryChar;
	TStr ClustKey = ClustChar;
	TInt Limit = TInt(limit);

	env->ReleaseStringUTFChars(query, QueryChar);
	env->ReleaseStringUTFChars(clustering, ClustChar);

	TStr QueryId = SearchPoint->GenQueryId(QueryStr, ClustKey, Limit);

	return env->NewStringUTF(QueryId.CStr());
}

JNIEXPORT void JNICALL JNICALL Java_si_ijs_searchpoint_SearchPoint_close(JNIEnv * env, jobject thisObjt) {
	Notify->OnNotify(TNotifyType::ntInfo, "Destroying SearchPoint ...");

	delete SearchPoint;

	Notify->OnNotify(TNotifyType::ntInfo, "Done!");
}
