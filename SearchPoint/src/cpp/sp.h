#ifndef SEARCH_POINT_H_
#define SEARCH_POINT_H_

#include <base.h>
#include <mine.h>
#include <thread.h>

namespace TSp {

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
	TSpItem();
	TSpItem(const int& Id, const TStr& Title, const TStr& Desc,
			const TStr& Url, const TStr& DispUrl);

    // SERIALIZATION
    TSpItem(TSIn&);
    void Save(TSOut&) const;

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

	TIntV ChildIdxV;

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
	TSpCluster(const TUInt64StrKd& _RecIdTopKwKd, const TVec<TKeyDat<TUInt64, TStrFltFltTr>>& _RecIdKwWgtFqTrKdV, const TFltPr& _Position, const int& _Size, const TIntV& _ChildIdxV):
		RecIdTopKwKd(_RecIdTopKwKd),
		RecIdKwWgtFqTrKdV(_RecIdKwWgtFqTrKdV),
		Pos(_Position),
		Size(_Size),
		ChildIdxV(_ChildIdxV) {}

    // SERIALIZATION
    TSpCluster(TSIn&);
    void Save(TSOut&) const;

  virtual ~TSpCluster() {}
};
typedef TVec<TSpCluster> TSpClusterV;

///////////////////////////////////////////////
// SearchPoint-Results
class TSpResult {
public:
	TStr QueryId;
	TStr QueryStr;
	TSpItemV ItemV;
	THash<TStr, TSpClusterV> ClusterVH;
	THash<TInt, TVec<double>> DocIdClustSimH;
	bool HasBackgroundClust;

    TSpResult() {}
	TSpResult(const TStr& _QueryId, const TStr& _QueryStr): QueryId(_QueryId), QueryStr(_QueryStr), HasBackgroundClust(false) {}
	TSpResult(const TStr& _QueryId, const TStr& _QueryStr, const THash<TStr, TSpClusterV> _ClustVH): QueryId(_QueryId), QueryStr(_QueryStr), ClusterVH(_ClustVH), HasBackgroundClust(false) {}
	TSpResult(const TStr& _QueryId, const TStr& _QueryStr, const TSpItemV& _ItemV, const THash<TStr, TSpClusterV>& _ClustVH, const THash<TInt, TVec<double>>& _DocClustSimH) :
		QueryId(_QueryId), QueryStr(_QueryStr), ItemV(_ItemV), ClusterVH(_ClustVH), DocIdClustSimH(_DocClustSimH), HasBackgroundClust(false) { }

	int GetItems() const { return ItemV.Len(); }
	int GetClusters() const { return ClusterVH.Len(); }
	bool HasClusters() const {
		TVec<TSpClusterV> DatV;	ClusterVH.GetDatV(DatV);
		for(int i=0; i<DatV.Len(); i++) if (DatV[i].Len()==0) { return false; } return true; }
};

///////////////////////////////////////////////
// SearchPoint-Utilities
class TSpUtils {
public:
	static void CalcPosition(const double StartX, const double StartY, const double Angle,
							const double Radius, TFltPr& Pos);

    template <typename TPermFun>
    static void Permute(TIntV& IdV, const TPermFun& PermFun);
    static int Factorial(const int n) {
        int Val = 1;
        for (int CurrVal = 2; CurrVal <= n; ++CurrVal) {
            Val *= CurrVal;
        }
        return Val;
    }

	// Transforms all the points in PointV to interval [MinX, MaxX]x[MinY, MaxY]
	static void TransformInterval(TVec<TFltV>& PointV, double MinX, double MaxX, double MinY, double MaxY);

    virtual ~TSpUtils() {}

private:
    template <typename TPermFun>
    static void Permute(TIntV& IdV, const int& StartN, const TPermFun& PermFun);
};

template <typename TPermFun>
void TSpUtils::Permute(TIntV& IdV, const TPermFun& PermFun) {
    Permute(IdV, 0, PermFun);
}

template <typename TPermFun>
void TSpUtils::Permute(TIntV& IdV, const int& StartN, const TPermFun& PermFun) {
    if (StartN == IdV.Len()) {
        PermFun(IdV);
    } else {
        for (int SwapN = StartN; SwapN < IdV.Len(); ++SwapN) {
            std::swap(IdV[StartN], IdV[SwapN]);
            Permute(IdV, StartN+1, PermFun);
            std::swap(IdV[StartN], IdV[SwapN]);
        }
    }
}

///////////////////////////////////////////////
// SearchPoint-Clustering-Utilities
class TSpClustUtils {
public:
	const static int MAX_CLUST_RADIUS;
	const static int MIN_CLUST_RADIUS;
	const static int NSamples;

public:
	virtual void CalcClusters(const TSpItemV& ItemV, TSpClusterV& ClusterV,
            TFltVV& DocClustSimVV, bool& HasBgClust) = 0;

	virtual ~TSpClustUtils() {}
};

///////////////////////////////////////////////
// SearchPoint KMeans Clustering Utilities
class TSpKMeansClustUtils : public TSpClustUtils {
private:
	static const int NClusters;
	static const int MinItemsPerCluster;
	static const int KwsPerClust;
	static const double MaxJoinDist;

public:
	TSpKMeansClustUtils() {}

protected:
	void CalcClusters(const TSpItemV& ItemV, TSpClusterV& ClusterV,
            TFltVV& DocClustSimVV, bool&);
private:
	void JoinClusters(const TVec<TFltV>& PosV, THash<TInt, TIntV>& MergedH, const double& MinJoinDist = .15) const;
};

///////////////////////////////////////////////
// SearchPoint DP-Means Clustering Utilities
class TSpDPMeansClustUtils : public TSpClustUtils {
private:
	static const int KwsPerClust;
    static const double MaxJoinDist;
	
	const double Lambda;
	const int MaxClusts;
	const int MinDocsPerClust;

	PNotify Notify;

public:
	TSpDPMeansClustUtils(const PNotify& _Notify=TNullNotify::New(), const double& _Lambda = .05, const int _MaxClusts = 7, const int& _MinDocsPerClust = 5):
		Lambda(_Lambda), MaxClusts(_MaxClusts), MinDocsPerClust(_MinDocsPerClust), Notify(_Notify) {}
protected:
	void CalcClusters(const TSpItemV& ItemV, TSpClusterV& ClusterV,
            TFltVV& DocClustSimVV, bool& HasBgClust);
private:
    void JoinClusters(const TVec<TFltV>& PosV, THash<TInt, TIntV>& MergedH, const double& MinJoinDist = .15) const;
};

///////////////////////////////////////////////
// SearchPoint-DMoz-Clustering-Utilities
class TSpDmozClustUtils : public TSpClustUtils {
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
	TSpDmozClustUtils(const TStr& DmozFilePath);

	void CalcClusters(const TSpItemV& ItemV, TSpClusterV& ClusterV, TFltVV& DocClustSimVV, bool&);

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
// SearchPoint
class TSpSearchPoint {
public:
    using TClustUtilH = THash<TStr, TSpClustUtils*>;    // TODO change this to something else

	const static double MaxClusterX;
	const static double MaxClusterY;

	const static double MinDist;

private:
	TClustUtilH ClustUtilsH;

	TStr DefaultClustUtilsKey;
	int PerPage;

	TCriticalSection CacheSection;

	PNotify Notify;

public:
	TSpSearchPoint(const TClustUtilH& ClustUtilsH,
			const TStr& DefaultClustUtilsKey, const int& _PerPage,
			const PNotify& Notify=TNullNotify::New());

    virtual ~TSpSearchPoint() {
        int KeyId = ClustUtilsH.FFirstKeyId();
        while (ClustUtilsH.FNextKeyId(KeyId)) {
            delete ClustUtilsH[KeyId];
        }
    }
    
	virtual PJsonVal CreateClusterJSon(const TSpCluster& Cluster) const;

public:
	/////////////////////////////////////////////////////////////////////////////////
	// If the query isn't cached, it computes clusters and similarities and creates
	// and caches a new query and returns it's result.
	// If the query is already cached, it just returns it's result.
	void GenClusters(const TStr& WidgetKey, const TSpItemV& ItemV,
            TSpClusterV& ClustV, TFltVV& DocClustSimVV, bool& HasBgClust);
	void ProcPosPageRq(const TFltPrV& PosV, const TSpClusterV& ClusterV,
            const TFltVV& ItemClustSimVV, const bool& HasBgClust, const int& Page,
            TIntV& PermuteV);
	PJsonVal ProcessPosKwRq(const TFltPr& Pos, const TSpClusterV& ClustV,
            const bool& HasBgClust);



	virtual PJsonVal GenJson(const TSpResult& SpResult, const int& Offset = 0, const int& Limit = -1, const bool& SummaryP = false) const;
	virtual PJsonVal GenItemsJSon(const TSpItemV& ItemV, const int& Offset = 0, const int& Limit = -1) const;
	virtual PJsonVal GenClustJSon(const THash<TStr, TSpClusterV>& ClusterVH) const;

	/////////////////////////////////////////////////////////////////////////////////
	// Calculates a weighted sum of the distance and similarity to each cluster and  
	// sorts the ResultItemV accordingly
	virtual void GetResultsByWSim(const TFltPrV& PosV, const TSpClusterV& ClustV, 
            const TFltVV& ItemClustSimVV, const bool& HasBgClust,
            const int& Offset, const int& Limit, TIntV& PermuteV) const = 0;
	virtual void GetKwsByWSim(const TFltPr& PosPr, const TSpClusterV& ClustV,
            const bool& HasBgClust, TStrV& KwV) const = 0;
};


///////////////////////////////////////////////
// SearchPoint-Implementation
class TSpSearchPointImpl : public TSpSearchPoint {
public:
	TSpSearchPointImpl(
            const TClustUtilH& ClustUtilsH,
            const TStr& DefaultClustUtilsKey,
            const int& PerPage,
            const PNotify& Notify=TNullNotify::New()
    );

	/* static PSpSearchPoint New(PSpClustUtils& PClustUtils, const int& PerPage, const PSpDataSource& DataSource, */
	/* 		const PNotify& Notify=TNullNotify::New()); */
	/* static PSpSearchPoint New(THash<TStr, PSpClustUtils>& ClustUtilsH, TStr& DefaultUtilsKey, const int& PerPage, */
	/* 		const PSpDataSource& DataSource, const PNotify& Notify=TNullNotify::New()); */

	static bool KwSuitable(const TStr& Kw, const TStrV& KwV);

	void GetResultsByWSim(const TFltPrV& PosV, const TSpClusterV& ClustV, 
            const TFltVV& ItemClustSimVV, const bool& HasBgClust,
            const int& Offset, const int& Limit, TIntV& PermuteV) const;
	void GetKwsByWSim(const TFltPr& PosPr, const TSpClusterV& ClustV,
            const bool& HasBgClust, TStrV& KwV) const;
};
}

#endif
