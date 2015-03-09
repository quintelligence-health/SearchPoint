#ifndef SEARCH_POINT_H_
#define SEARCH_POINT_H_

#include <base.h>
#include <mine.h>
#include <thread.h>

///////////////////////////////////////////////
// SearchPoint-Result-Item
class TSpItem {
public:
	int Id;
	TStr Title;
	TStr Description;
	TStr Url;
	TStr DisplayUrl;
	TStr DateTime;
public:
	TSpItem(): Id(), Title(), Description(), Url(), DisplayUrl(), DateTime() {}
	TSpItem(const int& _Id, const TStr& _Title, const TStr _Description, const TStr _Url, const TStr _DisplayUrl) :
		Id(_Id), Title(_Title), Description(_Description), Url(_Url), DisplayUrl(_DisplayUrl) {}

  virtual ~TSpItem() {}
};
typedef TVec<TSpItem> TSpItemV;

///////////////////////////////////////////////
// SearchPoint-Cluster
class TSpCluster {
public:
	TUInt64StrKd RecIdTopKwKd;
	TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtFqTrKdV;
	PBowDocPartClust BowDocPartClust;
	TFltPr Pos;
	int Size;

	TVec<int> ChildIdxV;

public:
	TSpCluster():
		RecIdTopKwKd(),
		RecIdKwWgtFqTrKdV(),
		BowDocPartClust(),
		Pos(),
		Size(),
		ChildIdxV() {}
	TSpCluster(const TUInt64StrKd& _RecIdTopKwKd, const TVec<TKeyDat<TUInt64, TStrFltFltTr>>& _RecIdKwWgtFqTrKdV, const TFltPr& _Position, const int& _Size):
		RecIdTopKwKd(_RecIdTopKwKd),
		RecIdKwWgtFqTrKdV(_RecIdKwWgtFqTrKdV),
		Pos(_Position),
		Size(_Size) {}
	TSpCluster(const TUInt64StrKd& _RecIdTopKwKd, const TVec<TKeyDat<TUInt64, TStrFltFltTr>>& _RecIdKwWgtFqTrKdV, const TFltPr& _Position, const int& _Size, const TVec<int>& _ChildIdxV):
		RecIdTopKwKd(_RecIdTopKwKd),
		RecIdKwWgtFqTrKdV(_RecIdKwWgtFqTrKdV),
		Pos(_Position),
		Size(_Size),
		ChildIdxV(_ChildIdxV) {}

  virtual ~TSpCluster() {}
};
typedef TVec<TSpCluster> TSpClusterV;

///////////////////////////////////////////////
// SearchPoint-Results
ClassTP(TSpResult, PSpResult)//{
public:
	const TStr QueryId;
	const TStr QueryStr;
	TSpItemV ItemV;
	THash<TStr, TSpClusterV> ClusterVH;
	THash<TInt, TVec<double>> DocIdClustSimH;
	bool HasBackgroundClust;

protected:
	TSpResult(const TStr& _QueryId, const TStr& _QueryStr): QueryId(_QueryId), QueryStr(_QueryStr), HasBackgroundClust(false) {}
	TSpResult(const TStr& _QueryId, const TStr& _QueryStr, const THash<TStr, TSpClusterV> _ClustVH): QueryId(_QueryId), QueryStr(_QueryStr), ClusterVH(_ClustVH), HasBackgroundClust(false) {}
	TSpResult(const TStr& _QueryId, const TStr& _QueryStr, const TSpItemV& _ItemV, const THash<TStr, TSpClusterV>& _ClustVH, const THash<TInt, TVec<double>>& _DocClustSimH) :
		QueryId(_QueryId), QueryStr(_QueryStr), ItemV(_ItemV), ClusterVH(_ClustVH), DocIdClustSimH(_DocClustSimH), HasBackgroundClust(false) { }
public:
	static PSpResult New(const TStr& _QueryId, const TStr& _QueryStr) { return new TSpResult(_QueryId, _QueryStr); }
	static PSpResult New(const TStr& _QueryId, const TStr& _QueryStr, const THash<TStr, TSpClusterV> _ClustVH) { return new TSpResult(_QueryId, _QueryStr, _ClustVH); }
	static PSpResult New(const TStr& _QueryId, const TStr& _QueryStr, const TSpItemV& ResultItemV, const THash<TStr, TSpClusterV>& _ClustVH, 
		const THash<TInt, TVec<double>>& RankClustSimH) { return new TSpResult(_QueryId, _QueryStr, ResultItemV, _ClustVH, RankClustSimH); }

	int GetItems() const { return ItemV.Len(); }
	int GetClusters() const { return ClusterVH.Len(); }
	bool HasClusters() const {
		TVec<TSpClusterV> DatV;	ClusterVH.GetDatV(DatV);
		for(int i=0; i<DatV.Len(); i++) if (DatV[i].Len()==0) { return false; } return true; }
  
  virtual ~TSpResult() {}
};

///////////////////////////////////////////////
// SearchPoint-Utilities
class TSpUtils {
public:
	static void CalcPosition(const double StartX, const double StartY, const double Angle,
							const double Radius, TFltPr& Pos);

	// Transforms all the points in PointV to interval [MinX, MaxX]x[MinY, MaxY]
	static void TransformInterval(TVec<TFltV>& PointV, double MinX, double MaxX, double MinY, double MaxY);

  virtual ~TSpUtils() {}
};

///////////////////////////////////////////////
// SearchPoint-Clustering-Utilities
ClassTP(TSpClustUtils, PSpClustUtils)// {
public:
	const static int MAX_CLUST_RADIUS;
	const static int MIN_CLUST_RADIUS;
	const static int NSamples;

public:
	virtual void CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV) = 0;

	virtual ~TSpClustUtils() {}
};

///////////////////////////////////////////////
// SearchPoint KMeans Clustering Utilities
ClassTE(TSpKMeansClustUtils, TSpClustUtils)//{
private:
	static const int NClusters;
	static const int MinItemsPerCluster;
	static const int KwsPerClust;
	static const double MaxJoinDist;

public:
	TSpKMeansClustUtils() {}
	static PSpClustUtils New() { return new TSpKMeansClustUtils; }

protected:
	void CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV);
private:
	void JoinClusters(const TVec<TFltV>& PosV, THash<TInt, TIntV>& MergedH, const double& MinJoinDist = .15) const;
};

///////////////////////////////////////////////
// SearchPoint DP-Means Clustering Utilities
ClassTE(TSpDPMeansClustUtils, TSpClustUtils)//{
private:
	static const int KwsPerClust;
	
	const double Lambda;
	const int MaxClusts;
	const int MinDocsPerClust;

	PNotify Notify;

public:
	TSpDPMeansClustUtils(const PNotify& _Notify=TNullNotify::New(), const double& _Lambda = .05, const int _MaxClusts = 7, const int& _MinDocsPerClust = 5):
		Lambda(_Lambda), MaxClusts(_MaxClusts), MinDocsPerClust(_MinDocsPerClust), Notify(_Notify) {}
	static PSpClustUtils New(const PNotify& Notify=TNullNotify::New(), const double& _Lambda = .05, const int _MaxClusts = 7, const int& _MinDocsPerClust = 5) {
		return new TSpDPMeansClustUtils(Notify, _Lambda, _MaxClusts, _MinDocsPerClust);
	}
protected:
	void CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV);
};

///////////////////////////////////////////////
// SearchPoint-DMoz-Clustering-Utilities
ClassTE(TSpDmozClustUtils, TSpClustUtils)//{
private:
	class TSpDmozCluster {
	public:
		TStr Name;
		TStr Path;
		TStr Keywords;
		TFltPr Position;
		int Size;
		int NItems;
		THash<TStr, TSpDmozCluster> ChildH;

	public:
		TSpDmozCluster();
		void PruneNotIn(const THashSet<TStr>& ClustSet);
		void ToVector(TVec<TSpDmozCluster>& Vector) const;
		
	public:
		class TSpDmozClustCmp {
		public:
			TSpDmozClustCmp() {};
			bool operator () (const TSpDmozCluster& c1, const TSpDmozCluster& c2) const {
				return c1.NItems > c2.NItems;
			};
		};
	};
private:
	const static int MaxClusters;
	const static int MaxDepth;

	TDMozCfy DMozCfy;
	TCriticalSection CSec;


public:
	TSpDmozClustUtils(const TStr& DmozFilePath):
		DMozCfy(*TFIn::New(DmozFilePath)), CSec(TCriticalSectionType::cstRecursive) {}
	static PSpClustUtils New(const TStr& DmozFilePath) { return new TSpDmozClustUtils(DmozFilePath); }

	void CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV);

private:
	int Prune(TSpDmozCluster& Root);

	// Prunes the tree by limiting the max depth 
	int Prune(TSpDmozCluster& Root, int Depth, const int& MaxDepth);

	// Copies the tree into a vector
	void CalcPositions(TSpDmozCluster& Root, const int& NItems) const;
	void CalcPositions(TSpDmozCluster& Root, double Radius, double BranchAngle, int Depth, const int& NItems) const;
	int CountDescendants(TSpDmozCluster& Root) const;

	void ToSpClustV(const TVec<TSpDmozCluster>& DmozClustV, TSpClusterV& ClusterV) const;
};

///////////////////////////////////////////////
// SearchPoint-Search-Engine
ClassTP(TSpDataSource, PSpDataSource)//{
protected:
	PNotify Notify;
public:
	TSpDataSource(const PNotify& Notify=TStdNotify::New());

	virtual void ExecuteQuery(PSpResult& PResult, const int NResults = 200) = 0;
	virtual ~TSpDataSource() {}
};

///////////////////////////////////////////////
// SearchPoint-Bing-Search-Engine
ClassTE(TSpBingEngine, TSpDataSource)//{
private:
	const static int MaxResultsPerQuery;

	TStrV BingApiKeyV;

	TRnd Rnd;

public:
	TSpBingEngine(const TStrV& BingApiKeyV, const PNotify& Notify);
	static PSpDataSource New(const TStrV& BingApiKeyV, const PNotify& Notify)
		{ return new TSpBingEngine(BingApiKeyV, Notify); }

protected:
	void ExecuteQuery(PSpResult& PResult, const int NResults = 200);

private:
	int ParseResponseBody(const TStr& XmlData, TSpItemV& ItemV, const int& Offset) const;
	void CreateTagNameHash(THash<TStr, TChA>& TagHash) const;
	int FetchResults(const TStr& Query, TSpItemV& ItemV, const int& Limit, const int& Offset);

	static FILE* OpenPipe(const TStr& ApiKey, const TStr& Query,
			const int& Offset, const int& Limit, const PNotify& Notify);
	static void ClosePipe(FILE* Pipe);
};

///////////////////////////////////////////////
// SearchPoint-Query
ClassTP(TSpQuery, PSpQuery)//{
public:
	PSpResult PResult;

	const TStr Id;
	TSecTm TimeStamp;

protected:
	TSpQuery(const PSpResult& _PResult) : PResult(_PResult), Id(_PResult->QueryId),
		TimeStamp(TSecTm::GetCurTm()) {}

public:
	static PSpQuery New(const PSpResult& PResult) { return new TSpQuery(PResult); }
	void Timestamp() { this->TimeStamp = TSecTm::GetCurTm(); }

  virtual ~TSpQuery() {}
};

///////////////////////////////////////////////
// SearchPoint-Query Manager
ClassTP(TSpQueryManager, PSpQueryManager)//{
private:
	const static TUInt64 MaxIdleSecs;

	THash<TStr, PSpQuery> QueryH;

protected:
	TSpQueryManager() : QueryH() {}

public:
	static PSpQueryManager New() { return new TSpQueryManager; }

  bool IsQuery(const TStr& QueryId) const { return QueryH.IsKey(QueryId); }
	PSpQuery NewQuery(const PSpResult& PResult);
  virtual PSpQuery GetQuery(const TStr& QueryId);

  virtual ~TSpQueryManager() {}

private:
	void DelOutdated();
};

///////////////////////////////////////////////
// SearchPoint
ClassTP(TSpSearchPoint, PSpSearchPoint)//{
public:
	const static double MaxClusterX;
	const static double MaxClusterY;

	const static double MinDist;

	PSpQueryManager PQueryManager;
	THash<TStr, PSpClustUtils> ClustUtilsH;

	PSpDataSource DataSource;

	TStr DefaultClustUtilsKey;
	int PerPage;

	TCriticalSection CacheSection;

	PNotify Notify;

protected:
	TSpSearchPoint(const THash<TStr, PSpClustUtils>& ClustUtilsH,
			const TStr& DefaultClustUtilsKey, const int& _PerPage,
			const PSpDataSource& DataSource, const PSpQueryManager& QueryManager=TSpQueryManager::New(),
			const PNotify& Notify=TNullNotify::New());
    
	virtual PJsonVal CreateClusterJSon(const TSpCluster& Cluster) const;

public:
	/////////////////////////////////////////////////////////////////////////////////
	// If the query isn't cached, it computes clusters and similarities and creates
	// and caches a new query and returns it's result.
	// If the query is already cached, it just returns it's result.
	virtual PSpResult GenClusters(PSpResult& SpResult, const TStr& ClustUtilsKey = TStr());
	virtual PJsonVal ProcPosPageRq(const TFltPrV& PosV, const int& Page, const TStr& QueryID);
	virtual PJsonVal ProcessPosKwRq(const TFltPr& PosV, const TStr& QueryId);

private:
	virtual PSpResult CreateNewResult(const TStr& QueryId, const TStr& QueryStr) const;
	PSpResult GetCachedResult(const TStr& QueryId);
	void CacheResult(const PSpResult& Result);
	PSpResult GetResult(const TStr& QueryId, const TStr& QueryStr, const TInt& NResults);

public:
	TStr GenQueryId(const TStr& QueryStr, const TStr& ClusteringKey, const int& NResults) const;
	PSpResult ExecuteQuery(const TStr& QueryStr, const TStr& ClusteringKey, const TInt& NResults);


	virtual PJsonVal GenJSon(const PSpResult& SpResult, const int& Offset = 0, const int& Limit = -1, const bool& SummaryP = false) const;
	virtual PJsonVal GenItemsJSon(const TSpItemV& ItemV, const int& Offset = 0, const int& Limit = -1) const;
	virtual PJsonVal GenClustJSon(const THash<TStr, TSpClusterV>& ClusterVH) const;

	/////////////////////////////////////////////////////////////////////////////////
	// Calculates a weighted sum of the distance and similarity to each cluster and  
	// sorts the ResultItemV accordingly
	virtual void GetResultsByWSim(const TFltPrV& PosV, const PSpResult& SpResult, TSpItemV& ResultV, const int& Offset = 0, const int& Limit = -1) const = 0;
	virtual void GetResultsByWSim(const TFltPrV& PosV, const TStr& QueryId, TSpItemV& ResultV, const int& Offset = 0, const int& Limit = -1);
	virtual void GetKwsByWSim(const TFltPr& PosPr, const PSpResult& SpResult, TStrV& KwV) const = 0;

	virtual ~TSpSearchPoint() {}

private:
	bool IsResultCached(const TStr& QueryID);
};


///////////////////////////////////////////////
// SearchPoint-Implementation
ClassTE(TSpSearchPointImpl, TSpSearchPoint)//{	
public:
	TSpSearchPointImpl(THash<TStr, PSpClustUtils>& _ClustUtilsH, TStr& _DefaultClustUtilsKey, const int& _PerPage,
			const PSpDataSource& DataSource, const PNotify& Notify=TNullNotify::New());

	static PSpSearchPoint New(PSpClustUtils& PClustUtils, const int& PerPage, const PSpDataSource& DataSource,
			const PNotify& Notify=TNullNotify::New());
	static PSpSearchPoint New(THash<TStr, PSpClustUtils>& ClustUtilsH, TStr& DefaultUtilsKey, const int& PerPage,
			const PSpDataSource& DataSource, const PNotify& Notify=TNullNotify::New());

	static bool KwSuitable(const TStr& Kw, const TStrV& KwV);

	void GetResultsByWSim(const TFltPrV& PosV, const PSpResult& SpResult, TSpItemV& ResultV, const int& Offset = 0, const int& Limit = -1) const;
	void GetKwsByWSim(const TFltPr& PosPr, const PSpResult& SpResult, TStrV& KwV) const;
};



/////////////////////////////////////////////////
//// SearchPoint - Abstract Server
//ClassTE(TSpAbstractServer, TSASFunFPath)// {
//protected:
//	const static TStr QueryIDAttrName;
//	const static TStr SearchEngineAttrName;
//	const static TStr QueryAttrName;
//	const static TStr ClusteringUtilsAttrName;
//	const static TStr NResultsAttrName;
//	const static TStr SummaryPAttrName;
//	const static int NResultsDefault;
//	const static int NResultsMin;
//
//	PSpSearchPoint PSearchPoint;
//
//	TStr FPath;
//
//public:
//	const static int MaxClusterRadius;
//	const static int MinClusterRadius;
//
//public:
//	TSpAbstractServer(const TStr& BaseUrl, const TStr& Path, PSpSearchPoint _PSearchPoint):
//			TSASFunFPath(BaseUrl, Path),
//			PSearchPoint(_PSearchPoint),
//			FPath(Path) {}
//
//	PSIn ExecSIn(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr);
//
//protected:
//	virtual PSIn ProcessQuery(const TStr& QueryStr, const TStr& ClusteringKey, const int& NResults = 200);
//
//	virtual PSIn ProcessPosUpdate(const TFltPrV& PosV, const int& Page, const TStr& QueryID) const;
//	virtual PSIn ProcessKwsUpdate(const TFltPr& Pos, const TStr& QueryId) const;
//	virtual PSIn ProcHtmlPgRq(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr) = 0;
//
//
//	TStr GenQueryID(const TStr& QueryStr, const TStr& ClusteringKey, const int& NResults) const;
//
//	virtual PSpResult ExecuteQuery(const TStr& QueryStr, const TStr& ClusteringKey, const int NResults = 200);
//};
//
/////////////////////////////////////////////////
//// SearchPoint - Demo Server
//ClassTE(TSpDemoSrv, TSpAbstractServer)//{
//private:
//	const static TStr QueryTargetStr;
//	const static TStr TitleTargetStr;
//	const static TStr QueryIDTargetStr;
//	const static TStr ClusteringTargetStr;
//	const static TStr NResultsTargetStr;
//
//public:
//	TSpDemoSrv(const TStr& BaseUrl, const TStr& Path, const PSpSearchPoint& PSearchPoint);
//
//	static PSAppSrvFun New(const TStr& BaseUrl, const TStr& Path, const PSpSearchPoint& PSearchPoint) {
//		return new TSpDemoSrv(BaseUrl, Path, PSearchPoint);
//	}
//
//	PSIn ExecSIn(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr) {
//		return TSpAbstractServer::ExecSIn(FldNmValPrV, RqEnv, ContTypeStr);
//	}
//
//protected:
//	PSIn ProcHtmlPgRq(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr);
//};

#endif