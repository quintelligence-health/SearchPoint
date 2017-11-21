#include "sp.h"

using namespace TSp;

//const int TSpAbstractServer::NResultsDefault = 200;
//const int TSpAbstractServer::NResultsMin = 200;
//
//const int TSpAbstractServer::MaxClusterRadius = 35;
//const int TSpAbstractServer::MinClusterRadius = 8;
//
//const TStr TSpAbstractServer::QueryIDAttrName = "qid";
//const TStr TSpAbstractServer::QueryAttrName = "q";
//const TStr TSpAbstractServer::ClusteringUtilsAttrName = "c";
//const TStr TSpAbstractServer::NResultsAttrName = "n";
//const TStr TSpAbstractServer::SummaryPAttrName = "sum";
//
//const TStr TSpDemoSrv::QueryTargetStr = "${query}";
//const TStr TSpDemoSrv::TitleTargetStr = "${title}";
//const TStr TSpDemoSrv::ClusteringTargetStr = "${clustering}";
//const TStr TSpDemoSrv::QueryIDTargetStr = "${queryID}";
//const TStr TSpDemoSrv::NResultsTargetStr = "${nresults}";


const double TSpSearchPoint::MaxClusterX = 1.0;
const double TSpSearchPoint::MaxClusterY = 1.0;

const int TSpClustUtils::MAX_CLUST_RADIUS = 35;
const int TSpClustUtils::MIN_CLUST_RADIUS = 8;
const int TSpClustUtils::NSamples = 200;

const int TSpKMeansClustUtils::NClusters = 8;
const int TSpKMeansClustUtils::MinItemsPerCluster = 10;
const int TSpKMeansClustUtils::KwsPerClust = 20;
const double TSpKMeansClustUtils::MaxJoinDist = .27;

const int TSpDmozClustUtils::MaxClusters = 15;
const int TSpDmozClustUtils::MaxDepth = 2;

const int TSpDPMeansClustUtils::KwsPerClust = 20;
const double TSpDPMeansClustUtils::MaxJoinDist = .27;

const double TSpSearchPoint::MinDist = 1e-11;


TSpItem::TSpItem():
		Id(),
		Title(),
		Description(),
		Url(),
		DisplayUrl(),
		DateTime() {}

TSpItem::TSpItem(const int& _Id, const TStr& _Title, const TStr& _Desc,
			const TStr& _Url, const TStr& _DispUrl):
		Id(_Id),
		Title(_Title),
		Description(_Desc),
		Url(_Url),
		DisplayUrl(_DispUrl),
		DateTime() {}

///////////////////////////////////////////////
// SearchPoint-Utilities
void TSpUtils::CalcPosition(const double StartX, const double StartY, const double Angle,
							const double Radius, TFltPr& Pos) {

	double x = StartX + Radius*sin(Angle);
	double y = StartY + Radius*cos(Angle);

	Pos.Val1 = x;
	Pos.Val2 = y;
}

void TSpUtils::TransformInterval(TVec<TFltV>& PointV, double MinX, double MaxX, double MinY, double MaxY) {
	double MaxXOld = TFlt::NInf, MaxYOld = TFlt::NInf;
	double MinXOld = TFlt::PInf, MinYOld = TFlt::PInf;
	for (int i = 0; i < PointV.Len(); i++) {
		TFltV& Point = PointV[i];
		double x = Point[0];
		double y = Point[1];

		if (x > MaxXOld)
			MaxXOld = x;
		if (x < MinXOld)
			MinXOld = x;
		if (y > MaxYOld)
			MaxYOld = y;
		if (y < MinYOld)
			MinYOld = y;
	}

	double Dx = MaxX - MinX;
	double Dy = MaxY - MinY;
	double DxOld = MaxXOld - MinXOld;
	double DyOld = MaxYOld - MinYOld;

	//this probably can be done faster, but it should work
	for (int i = 0; i < PointV.Len(); i++) {
		TFltV& Point = PointV[i];
		double x = Point[0];
		double y = Point[1];

		Point[0] = Dx*(x - MinXOld)/DxOld + MinX;
		Point[1] = Dy*(y - MinYOld)/DyOld + MinY;
	}
}

///////////////////////////////////////////////
// SearchPoint-KMeans-Clustering-Utilities
void TSpKMeansClustUtils::JoinClusters(const TVec<TFltV>& PosV, THash<TInt, TIntV>& MergedH, const double& MinJoinDist) const {
    // join the clusers which are too close together into a single cluster
    // create a distance matrix
    MergedH.Clr();
    THash<TInt, TIntFltH> DistMat;
    for (TInt i = 0; i < PosV.Len(); i++) {
        TIntFltH Row;
        for (TInt j = 0; j < PosV.Len(); j++) {
            TFltV p1 = PosV[i];
            TFltV p2 = PosV[j];

            Row.AddDat(j, TMath::Sqrt(TMath::Sqr(p1[0] - p2[0]) + TMath::Sqr(p1[1] - p2[1])));
        }

        DistMat.AddDat(i, Row);
    }
    // quasi hierarchical clustering
    // picks the forst pair with dist <= MinDist and joins them
    bool change = true;

    TIntV CurrKeyV;        DistMat.GetKeyV(CurrKeyV);
    while (change) {
        change = false;
        for (int i = 0; i < CurrKeyV.Len(); i++) {
            const int& RowKey = CurrKeyV[i];
            const TIntFltH& Row = DistMat(RowKey);

            for (int j = 0; j < CurrKeyV.Len(); j++) {
				const int& ColKey = CurrKeyV[j];
				if (RowKey == ColKey) continue;
                //if (i == j) continue;
         
                const double& Dist = Row.GetDat(ColKey);
                if (Dist < MinJoinDist) {
                    // adjust the matrix
                    change = true;

                    const int MinKey = TMath::Mn(RowKey, ColKey);
                    const int MaxKey = TMath::Mx(RowKey, ColKey);

                    // compute avg dist and remove the cluster with the higher key
//                    TIntFltH& MinRow = DistMat.GetDat(MinKey);
//                    TIntFltH& MaxRow = DistMat.GetDat(MaxKey);
                    for (int k = 0; k < CurrKeyV.Len(); k++) {
                            if (k == MinKey || k == MaxKey) continue;

                            DistMat(MinKey)(k) = (DistMat(MinKey)(k) + DistMat(MaxKey)(k))/2;
                            DistMat(k)(MinKey) = (DistMat(k)(MinKey) + DistMat(k)(MaxKey))/2;

                            DistMat(k).DelIfKey(MaxKey);
                    }
                    DistMat.DelIfKey(MaxKey);

                    // remember the merge
                    if (!MergedH.IsKey(MinKey)) {
						TIntV MergedV;        
						MergedH.AddDat(MinKey, MergedV);
					}
                    MergedH(MinKey).Add(MaxKey);
                    //remove the max key from the list of current keys
                    CurrKeyV.DelIfIn(MaxKey);
                    break;
                }
            }

            if (change) break;
        }
    }

    for (int i = 0; i < CurrKeyV.Len(); i++) {
        const int& ClustIdx = CurrKeyV[i];
        if (!MergedH.IsKey(ClustIdx)) {
                TIntV EmptyV;
                MergedH.AddDat(ClustIdx, EmptyV);
        }
    }
}

void TSpKMeansClustUtils::CalcClusters(const TSpItemV& ItemV,
        TSpClusterV& ClusterV, TFltVV& DocClustSimVV, bool&) {
	/* const TSpItemV& ItemV = SpResult.ItemV; */
	/* THash<TInt, TVec<double>>& SimsH =  SpResult.DocIdClustSimH; */

	/* SimsH.Clr(); */

	if (ItemV.Len() < 3*TSpKMeansClustUtils::MinItemsPerCluster)	// check if enough clusters
		return;

	const int Clusts = TMath::Mn((int) ceil((double) ItemV.Len() / TSpKMeansClustUtils::MinItemsPerCluster), TSpKMeansClustUtils::NClusters);

//	int NSamples = min(ItemV.Len(), TSpClustUtils::NSamples);

	//copying the relevant text into a vector of snippets
	TStrV SnippetV;
	for (int i = 0; i < ItemV.Len(); i++)
		SnippetV.Add(ItemV[i].Title + " " + ItemV[i].Description);

	// prepare list of stopwords (words to be ignored)
	PSwSet SwSet = TSwSet::New(swstEn523);
	// prepare stemmer (cuts of endings from english documents)
	PStemmer Stemmer = TStemmer::New(stmtPorter, true);
	// computes n-grams (e.g. New York) of lenght up to 3, which occurr at least 5 times
	PNGramBs NGramBs = TNGramBs::GetNGramBsFromHtmlStrV(SnippetV, 3, 5, SwSet, Stemmer);
	// create empty bag-of-words object
	PBowDocBs BowDocBs = TBowDocBs::New(SwSet, Stemmer, NGramBs);
	// load sinppets into the BOW object
	for (int SnippetN = 0; SnippetN < SnippetV.Len(); SnippetN++) {
		BowDocBs->AddHtmlDoc(TInt::GetStr(SnippetN), TStrV(), SnippetV[SnippetN]);
	}

	// compute TFIDF weights
	PBowDocWgtBs BowDocWgtBs = TBowDocWgtBs::New(BowDocBs, bwwtNrmTFIDF);
	// prepare object for computing similarity between sparse vectors
	PBowSim BowSim = TBowSim::New(bstCos);
	// does the clustering into clusters
	TRnd Rnd(1);
	PBowDocPart BowDocPart = TBowClust::GetKMeansPartForDocWgtBs(TStdNotify::New(),
		BowDocWgtBs, BowDocBs, BowSim, Rnd, Clusts, 1, 10, 5);

	// transforming the clusters onto a 2D space
	TVec<TFltV> DocPointV;
	int NDocs = BowDocPart->GetClusts();
	TVec<PBowSpV> ClustSpV(NDocs+1, 0);
	for (int i = 0; i < NDocs; i++) {
		PBowDocPartClust PClust = BowDocPart->GetClust(i);
		ClustSpV.Add(PClust->GetConceptSpV());
	}

	// adding an 'invisible cluster' which will hopefully be in the center
	TIntV AllDIdV; BowDocBs->GetAllDIdV(AllDIdV);
	PBowSpV AllSpV = TBowClust::GetConceptSpV(BowDocWgtBs, BowSim, AllDIdV);
	ClustSpV.Add(AllSpV);

	PNotify Notify = new TNotify();
	PSVMTrainSet ClustSet = TBowDocBs2TrainSet::NewBowNoCat(ClustSpV);
	TVizMapFactory::MakeFlat(ClustSet, vdtEucl, DocPointV, 10000, 1, 0, true, Notify);

	DocPointV.Del(DocPointV.Len()-1);

	// transforming to the canvas interval
	TSpUtils::TransformInterval(DocPointV, 0.0, TSpSearchPoint::MaxClusterX, 0.0,
		TSpSearchPoint::MaxClusterY);

    // merge the clusters which are too close
	THash<TInt, TIntV> MergedH;	JoinClusters(DocPointV, MergedH, TSpKMeansClustUtils::MaxJoinDist);
	TIntV TopClustIdV;	MergedH.GetKeyV(TopClustIdV);

	for (int AggClustN = 0; AggClustN < TopClustIdV.Len(); AggClustN++) {
		const int& TopClustId = TopClustIdV[AggClustN];
		TIntV& MergedClustIdV = MergedH.GetDat(TopClustId);
		MergedClustIdV.Ins(0, TopClustId);

		// compute the avg coordinates
		const int NClusts = MergedClustIdV.Len();
		TFltPr AvgCoords(0, 0);
		for (int j = 0; j < NClusts; j++) {
			const int& ClustIdx = MergedClustIdV[j];
			const TFltV& ClustPos = DocPointV[ClustIdx];

			AvgCoords.Val1 += ClustPos[0] / NClusts;
			AvgCoords.Val2 += ClustPos[1] / NClusts;
		}

		const int KwsInJointClust = (int) (2*TSpKMeansClustUtils::KwsPerClust*(1.0 - TMath::Power(2, -NClusts)));	// KwsPerClust * Sum[2^(-AggClustN), {AggClustN,0,NClusts-1}]
		const int NKwsDefault = KwsInJointClust / NClusts;

		// get the keywords
		TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtFqTrKdV(NKwsDefault * NClusts, 0);
		int CurrClustId = 0;
		for (int MergedClustN = 0; MergedClustN < NClusts; MergedClustN++) {
			const int ClustIdx = MergedClustIdV[MergedClustN];
			PBowDocPartClust PClust = BowDocPart->GetClust(ClustIdx);

			// get keywords
			PBowSpV ClustSpV = PClust->GetConceptSpV();
			PBowKWordSet ClustKWordSet = ClustSpV->GetKWordSet(BowDocBs)->
				GetTopKWords(NKwsDefault, 1.0);

			const int NKws = TMath::Mn(ClustKWordSet->GetKWords(), NKwsDefault);

			for (int k = 0; k < NKws; k++) {
				const TStr Kw = ClustKWordSet->GetKWordStr(k);
				const TFlt KwWgt = ClustKWordSet->GetKWordWgt(k);
				const double GlobalKWordFq = BowDocWgtBs->GetWordFq(BowDocBs->GetWId(Kw));	// TODO check if this is correct
				
				RecIdKwWgtFqTrKdV.Add(TKeyDat<TUInt64, TStrFltFltTr>(
					(uint64) CurrClustId++, TStrFltFltTr(Kw, KwWgt, GlobalKWordFq)));
			}
		}

		int Size = (int) (TSpClustUtils::MAX_CLUST_RADIUS *
			TMath::Sqrt(2.0 - TMath::Power(2, -(NClusts - 1))));

		TSpCluster Cluster(TUInt64StrKd((uint64) AggClustN, RecIdKwWgtFqTrKdV[0].Dat.Val1), RecIdKwWgtFqTrKdV,
			TFltPr(AvgCoords.Val1, AvgCoords.Val2), Size);
		ClusterV.Add(Cluster);

		// compute similarities
		for (int MergedClustN = 0; MergedClustN < NClusts; MergedClustN++) {
			const int& ClustN = MergedClustIdV[MergedClustN];
			PBowDocPartClust Clust = BowDocPart->GetClust(ClustN);
			PBowSpV ClustSpV = Clust->GetConceptSpV();

			// for each document calculate the similarity to this cluster
			for (int DocN = 0; DocN < BowDocWgtBs->GetDocs(); DocN++) {
				const int& DocId = BowDocWgtBs->GetDId(DocN);
				const int& SnippetN = BowDocBs->GetDocNm(DocId).GetInt();

				PBowSpV DocSpV = BowDocWgtBs->GetSpV(DocId);
				const TFlt ClustDocSim = BowSim->GetSim(ClustSpV, DocSpV);

                DocClustSimVV(SnippetN, AggClustN) += ClustDocSim / NClusts;
				/* DocSimV[AggClustN] += ClustDocSim / NClusts; */
			}
		}
	}
}

///////////////////////////////////////////////
// SearchPoint DP-Means Clustering Utilities
void TSpDPMeansClustUtils::CalcClusters(const TSpItemV& ItemV,
        TSpClusterV& ClusterV, TFltVV& DocClustSimVV, bool& HasBgClust) {
	Notify->OnNotify(TNotifyType::ntInfo, "Computing clusters...");

	/* const TSpItemV& ItemV = SpResult.ItemV; */
	/* THash<TInt, TVec<double>>& SimsH =  SpResult.DocIdClustSimH; */

	if (ItemV.Empty()) { return; }

/* 	SimsH.Clr(); */

	//copying the relevant text into a vector of snippets
	TStrV SnippetV;
	for (int i = 0; i < ItemV.Len(); i++)
		SnippetV.Add(ItemV[i].Title + " " + ItemV[i].Description);

	// prepare list of stopwords (words to be ignored)
	PSwSet SwSet = TSwSet::New(swstEn523);
	// prepare stemmer (cuts of endings from english documents)
	PStemmer Stemmer = TStemmer::New(stmtPorter, true);
	// computes n-grams (e.g. New York) of lenght up to 3, which occurr at least 5 times
	PNGramBs NGramBs = TNGramBs::GetNGramBsFromHtmlStrV(SnippetV, 3, 5, SwSet, Stemmer);
	// create empty bag-of-words object
	PBowDocBs BowDocBs = TBowDocBs::New(SwSet, Stemmer, NGramBs);
	// load sinppets into the BOW object
	for (int SnippetN = 0; SnippetN < SnippetV.Len(); SnippetN++) {
		BowDocBs->AddHtmlDoc(TInt::GetStr(SnippetN), TStrV(), SnippetV[SnippetN]);
	}

	// compute TFIDF weights
	PBowDocWgtBs BowDocWgtBs = TBowDocWgtBs::New(BowDocBs, bwwtNrmTFIDF);

	// prepare object for computing similarity between sparse vectors
	PBowSim BowSim = TBowSim::New(bstCos);
	
	Notify->OnNotify(TNotifyType::ntInfo, "Clustering ...");

	// does the clustering into clusters
	TRnd Rnd(1);
	PBowDocPart BowDocPart = TBowClust::GetDPMeansPartForDocWgtBs(Notify,
		BowDocWgtBs, BowDocBs, BowSim, Rnd, Lambda, MinDocsPerClust, MaxClusts, 1, 10000, TBowClustInitScheme::tbcKMeansPP, 4);

    Notify->OnNotify(TNotifyType::ntInfo, "Transforming onto a plane ...");

    //transforming the clusters onto a 2D space
    int NClusts = BowDocPart->GetClusts();

    TVec<PBowSpV> ClustSpVV(NClusts+1, 0);
    for (int i = 0; i < NClusts; i++) {
        PBowDocPartClust PClust = BowDocPart->GetClust(i);
        ClustSpVV.Add(PClust->GetConceptSpV());
    }

    //adding an 'invisible cluster' which will hopefully be in the center
    TIntV AllDIdV; BowDocBs->GetAllDIdV(AllDIdV);
    PBowSpV AllSpV = TBowClust::GetConceptSpV(BowDocWgtBs, BowSim, AllDIdV);
    ClustSpVV.Add(AllSpV);

    PNotify Notify = new TNotify();
    PSVMTrainSet ClustSet = TBowDocBs2TrainSet::NewBowNoCat(ClustSpVV);
	
    TIntV BestIdV;
    {
        double BestPermSim = TFlt::NInf;
        int PermN = 0;

        TIntV ClustIdV;
        for (int ClustN = 0; ClustN < NClusts; ++ClustN) {
            ClustIdV.Add(ClustN);
        }

        auto PermFun = [&](const TIntV& CurrIdV) {
            double TotalSim = 0;
            for (int ClustN = 1; ClustN < CurrIdV.Len(); ++ClustN) {
                TotalSim += ClustSet->DotProduct(CurrIdV[ClustN-1]+1, CurrIdV[ClustN]+1);
            }
            TotalSim += ClustSet->DotProduct(CurrIdV.Last()+1, CurrIdV[0]+1);
            if (TotalSim > BestPermSim) {
                BestPermSim = TotalSim;
                BestIdV = CurrIdV;
            }
            ++PermN;
        };
        TSpUtils::Permute(ClustIdV, PermFun);
        EAssert(PermN == TSpUtils::Factorial(NClusts));
    }
	// put the clusters in a circle
	TVec<TFltV> DocPointV(NClusts+1,0);
	double ThetaStart = TRnd(0).GetUniDev()*TMath::Pi*2;
	double ThetaStep = 2*TMath::Pi / NClusts;
	for (int i = 0; i < NClusts; i++) {
        const int ClustN = BestIdV[i];
		const double Theta = ThetaStart + ClustN*ThetaStep;
		
		TFltV PosV;
		PosV.Add(cos(Theta));
		PosV.Add(sin(Theta));

		DocPointV.Add(PosV);
	}

	TFltV PosV;
	PosV.Add(0);
	PosV.Add(0);
	DocPointV.Add(PosV);
	/* TVizMapFactory::MakeFlat(ClustSet, TVizDistType::vdtCos, DocPointV, 10000, 1, TMath::Pi / (2*NClusts), false, Notify); */
    TVizMapFactory::MakeFlat(ClustSet, TVizDistType::vdtCos, DocPointV, 10000, 1, 0, false, Notify);

    DocPointV.Del(DocPointV.Len()-1);

    Notify->OnNotify(TNotifyType::ntInfo, "Transforming to [0,1]x[0,1] ...");
    //transforming to the canvas interval
    TSpUtils::TransformInterval(DocPointV, 0.0, TSpSearchPoint::MaxClusterX,
		0.0, TSpSearchPoint::MaxClusterY);

    THash<TInt, TIntV> MergedH;	JoinClusters(DocPointV, MergedH, TSpDPMeansClustUtils::MaxJoinDist);
    TIntV TopClustIdV;	MergedH.GetKeyV(TopClustIdV);

    DocClustSimVV.Gen(ItemV.Len(), TopClustIdV.Len());

    int OutClustN = 0;
    for (int TopClustN = 0; TopClustN < TopClustIdV.Len(); ++TopClustN) {
        const int& TopClustId = TopClustIdV[TopClustN];
        TIntV& MergedClustIdV = MergedH.GetDat(TopClustId);
        MergedClustIdV.Ins(0, TopClustId);

        // compute the avg coordinates
        const int NClusts = MergedClustIdV.Len();
        TFltPr AvgCoords(0, 0);
        for (int MergedClustN = 0; MergedClustN < NClusts; MergedClustN++) {
            const int& ClustIdx = MergedClustIdV[MergedClustN];
            const TFltV& ClustPos = DocPointV[ClustIdx];

            AvgCoords.Val1 += ClustPos[0] / NClusts;
            AvgCoords.Val2 += ClustPos[1] / NClusts;
        }

        TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtFqTrKdV;
        TStrV KwV;

        for (int MergedClustN = 0; MergedClustN < NClusts; ++MergedClustN) {
            const int ClustId = MergedClustIdV[MergedClustN];
            PBowDocPartClust BowDocPartClust = BowDocPart->GetClust(ClustId);

            // get keywords
            PBowSpV ClustSpV = BowDocPartClust->GetConceptSpV();
            PBowKWordSet ClustKWordSet = ClustSpV->GetKWordSet(BowDocBs)->GetTopKWords(
                TSpDPMeansClustUtils::KwsPerClust, 1.0);
            
            /* const int NKws = TMath::Mn(ClustKWordSet->GetKWords(), TSpDPMeansClustUtils::KwsPerClust); */
            const int NKws = TMath::Mn(ClustKWordSet->GetKWords(), TSpDPMeansClustUtils::KwsPerClust);

            if (NKws > 1) {
                for (int KwN = 0; KwN < NKws; ++KwN) {
                    const TStr& Kw = ClustKWordSet->GetKWordStr(KwN);
                    const TFlt& KwWgt = ClustKWordSet->GetKWordWgt(KwN);
                    const double GlobalKWordFq = BowDocWgtBs->GetWordFq(BowDocBs->GetWId(Kw));	// TODO check if this is correct

                    if (!TSpSearchPointImpl::KwSuitable(Kw, KwV)) continue;

                    RecIdKwWgtFqTrKdV.Add(TKeyDat<TUInt64, TStrFltFltTr>(
                        (uint64) OutClustN, TStrFltFltTr(Kw, KwWgt, GlobalKWordFq)));
                    KwV.Add(Kw);
                }
            }
        }

        auto Cmp = [&](const TKeyDat<TUInt64, TStrFltFltTr>& Val1, const TKeyDat<TUInt64, TStrFltFltTr>& Val2) {
            return Val1.Dat.Val2 > Val2.Dat.Val2; };
        RecIdKwWgtFqTrKdV.SortCmp(Cmp);

        const double SizeFactor = TMath::Sqrt(2.0 - TMath::Power(2, -(NClusts - 1)));
        const int ClustUiSize = (int) (TSpClustUtils::MAX_CLUST_RADIUS * SizeFactor);
        const int MxKws = (int) std::ceil(TSpDPMeansClustUtils::KwsPerClust * SizeFactor);

        if (RecIdKwWgtFqTrKdV.Len() > MxKws) {
            RecIdKwWgtFqTrKdV.Trunc(MxKws);
        }

        if (RecIdKwWgtFqTrKdV.Len() > 1) {
            TSpCluster Cluster(TUInt64StrKd((uint64) OutClustN,
                        RecIdKwWgtFqTrKdV[0].Dat.Val1), RecIdKwWgtFqTrKdV,
                    TFltPr(AvgCoords.Val1, AvgCoords.Val2), ClustUiSize);
            ClusterV.Add(Cluster);

            // compute similarities to the documents
            for (int MergedClustN = 0; MergedClustN < NClusts; ++MergedClustN) {
                const int ClustId = MergedClustIdV[MergedClustN];
                PBowDocPartClust Clust = BowDocPart->GetClust(ClustId);
                PBowSpV ClustSpV = Clust->GetConceptSpV();

                // for each document calculate the similarity to this cluster
                for (int DocN = 0; DocN < BowDocWgtBs->GetDocs(); DocN++) {
                    const int& DocId = BowDocWgtBs->GetDId(DocN);
                    const int& SnippetN = BowDocBs->GetDocNm(DocId).GetInt();

                    PBowSpV DocSpV = BowDocWgtBs->GetSpV(DocId);
                    const TFlt ClustDocSim = BowSim->GetSim(ClustSpV, DocSpV);

                    DocClustSimVV(SnippetN, OutClustN) += ClustDocSim / NClusts;
                    /* DocSimV[OutClustN] += ClustDocSim / NClusts; */
                }
            }

            ++OutClustN;
        }
    }

	// create the central cluster
    Notify->OnNotify(TNotifyType::ntInfo, "Creating central cluster ...");

	const PBowKWordSet KwSet = AllSpV->GetKWordSet(BowDocBs)->GetTopKWords(50, 1);
	const int NKws = TMath::Mn(KwSet->GetKWords(), 50);

	TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtFqTrKdV(NKws, 0);
	TStrV KwV;

	for (int KwIdx = 0; KwIdx < NKws; KwIdx++) {
		const TStr Kw = KwSet->GetKWordStr(KwIdx);

		if (!TSpSearchPointImpl::KwSuitable(Kw, KwV)) continue;

		const TFlt KwWgt = KwSet->GetKWordWgt(KwIdx);
		const TFlt GlobalKWordFq = BowDocWgtBs->GetWordFq(BowDocBs->GetWId(Kw));

		RecIdKwWgtFqTrKdV.Add(TKeyDat<TUInt64, TStrFltFltTr>(
				(uint64) KwIdx, TStrFltFltTr(Kw, KwWgt, GlobalKWordFq)));
		KwV.Add(Kw);
	}

	TSpCluster Cluster(TUInt64StrKd((uint64) DocPointV.Len(), RecIdKwWgtFqTrKdV.Len() > 0 ? RecIdKwWgtFqTrKdV[0].Dat.Val1 : ""), RecIdKwWgtFqTrKdV, TFltPr(.5, .5),
			TSpClustUtils::MAX_CLUST_RADIUS);
	ClusterV.Add(Cluster);
	HasBgClust = true;

	Notify->OnNotify(TNotifyType::ntInfo, "Finished!");
}

void TSpDPMeansClustUtils::JoinClusters(const TVec<TFltV>& PosV, THash<TInt, TIntV>& MergedH, const double& MinJoinDist) const {
    // join the clusers which are too close together into a single cluster
    // create a distance matrix
    MergedH.Clr();
    THash<TInt, TIntFltH> DistMat;
    for (TInt i = 0; i < PosV.Len(); i++) {
        TIntFltH Row;
        for (TInt j = 0; j < PosV.Len(); j++) {
            TFltV p1 = PosV[i];
            TFltV p2 = PosV[j];

            Row.AddDat(j, TMath::Sqrt(TMath::Sqr(p1[0] - p2[0]) + TMath::Sqr(p1[1] - p2[1])));
        }

        DistMat.AddDat(i, Row);
    }
    // quasi hierarchical clustering
    // picks the forst pair with dist <= MinDist and joins them
    bool change = true;

    TIntV CurrKeyV;        DistMat.GetKeyV(CurrKeyV);
    while (change) {
        change = false;
        for (int i = 0; i < CurrKeyV.Len(); i++) {
            const int& RowKey = CurrKeyV[i];
            const TIntFltH& Row = DistMat(RowKey);

            for (int j = 0; j < CurrKeyV.Len(); j++) {
                const int& ColKey = CurrKeyV[j];
                if (RowKey == ColKey) continue;
                //if (i == j) continue;
         
                const double& Dist = Row.GetDat(ColKey);
                if (Dist < MinJoinDist) {
                    // adjust the matrix
                    change = true;

                    const int MinKey = TMath::Mn(RowKey, ColKey);
                    const int MaxKey = TMath::Mx(RowKey, ColKey);

                    // compute avg dist and remove the cluster with the higher key
//                    TIntFltH& MinRow = DistMat.GetDat(MinKey);
//                    TIntFltH& MaxRow = DistMat.GetDat(MaxKey);
                    for (int k = 0; k < CurrKeyV.Len(); k++) {
                            if (k == MinKey || k == MaxKey) continue;

                            DistMat(MinKey)(k) = (DistMat(MinKey)(k) + DistMat(MaxKey)(k))/2;
                            DistMat(k)(MinKey) = (DistMat(k)(MinKey) + DistMat(k)(MaxKey))/2;

                            DistMat(k).DelIfKey(MaxKey);
                    }
                    DistMat.DelIfKey(MaxKey);

                    // remember the merge
                    if (!MergedH.IsKey(MinKey)) {
                        TIntV MergedV;        
                        MergedH.AddDat(MinKey, MergedV);
                    }
                    MergedH(MinKey).Add(MaxKey);
                    //remove the max key from the list of current keys
                    CurrKeyV.DelIfIn(MaxKey);
                    break;
                }
            }

            if (change) break;
        }
    }

    for (int i = 0; i < CurrKeyV.Len(); i++) {
        const int& ClustIdx = CurrKeyV[i];
        if (!MergedH.IsKey(ClustIdx)) {
                TIntV EmptyV;
                MergedH.AddDat(ClustIdx, EmptyV);
        }
    }
}

///////////////////////////////////////////////
// SearchPoint-DMoz-Clustering-Utilities
TSpDmozClustUtils::TSpDmozCluster::TSpDmozCluster():
		Name(),
		Path(),
		Keywords(),
		Position(),
		Size(),
		NItems(),
		ChildH() {}

int TSpDmozClustUtils::Prune(TSpDmozCluster& Root, int Depth, const int& MaxDepth) {
	int Sum = 1;
	if (Depth < MaxDepth) {
		THash<TStr, TSpDmozCluster>& ChildH = Root.ChildH;
		TVec<TStr> KeyV;	ChildH.GetKeyV(KeyV);

		
		for (int i = 0; i < KeyV.Len(); i++) {
			TSpDmozCluster& Child = ChildH.GetDat(KeyV[i]);
			Sum += Prune(Child, Depth+1, MaxDepth);
		}
	} else {
		//delete all children
		Root.ChildH.Clr();
	}

	return Sum;
}

int TSpDmozClustUtils::Prune(TSpDmozCluster& Root) {
	return Prune(Root, 0, TSpDmozClustUtils::MaxDepth);
}

void TSpDmozClustUtils::CalcPositions(TSpDmozCluster& Root, double Radius,
	double BranchAngle, int Depth, const int& NItems) const {

	THash<TStr, TSpDmozCluster>& ChildH = Root.ChildH;

	if (Depth == 0) {
		//place root in the center
		double x = TSpSearchPoint::MaxClusterX/2;
		double y = TSpSearchPoint::MaxClusterY/2;

		Root.Position = TFltPr(x, y);
		Root.Size = TSpClustUtils::MAX_CLUST_RADIUS;

		TVec<TSpDmozCluster> ChildV;	ChildH.GetDatV(ChildV);
		int NChildren = ChildV.Len();

		if (NChildren == 0)
			return;

		double Angle = 2*TMath::Pi/NChildren;
		double CurrAngle = 0.0;
		ChildH.Clr();	//I don't know why it didn't work without this
		for (int i = 0; i < NChildren; i++) {
			TSpDmozCluster& Child = ChildV[i];
			TFltPr ChildPos;	
			TSpUtils::CalcPosition(x, y, CurrAngle, Radius, ChildPos);

			Child.Position = ChildPos;
			CalcPositions(Child, Radius/2, CurrAngle, Depth + 1, NItems);
			ChildH.AddDat(Child.Name, Child);

			CurrAngle += Angle;
		}
	} else {
		//calculate the size
		Root.Size = TMath::Mx((int) ((double) TSpClustUtils::MAX_CLUST_RADIUS*Root.NItems/NItems),
				TSpClustUtils::MIN_CLUST_RADIUS);

		TVec<TSpDmozCluster> ChildV;	Root.ChildH.GetDatV(ChildV);
		int NChildren = ChildV.Len();

		const TFltPr& RootPos = Root.Position;
		//place the children in the same direction as the whole branch
		double CurrAngle = BranchAngle - TMath::Pi/3;	//== CurrAngle - PI/2 + PI/6
		double Step = 2*TMath::Pi/(3*(NChildren-1));
		ChildH.Clr();	//I don't know why it didn't work without this
		for (int i = 0; i < NChildren; i++) {
			TSpDmozCluster& Child = ChildV[i];

			TFltPr ChildPos;	
			TSpUtils::CalcPosition(RootPos.Val1, RootPos.Val2, CurrAngle, Radius, ChildPos);
			Child.Position = ChildPos;

			CalcPositions(Child, Radius/2, CurrAngle, Depth + 1, NItems);
			ChildH.AddDat(Child.Name, Child);

			CurrAngle += Step;
		}
	}
}

void TSpDmozClustUtils::CalcPositions(TSpDmozCluster& Root, const int& NItems) const {
	double Radius = TMath::Mn(TSpSearchPoint::MaxClusterX, TSpSearchPoint::MaxClusterY)/2;
	CalcPositions(Root, Radius, 0.0, 0, NItems);
}

int TSpDmozClustUtils::CountDescendants(TSpDmozCluster& Root) const {
	THash<TStr, TSpDmozCluster>& ChildH = Root.ChildH;
	TVec<TStr> KeyV;	ChildH.GetKeyV(KeyV);

	int Sum = Root.NItems;
	for (int i = 0; i < KeyV.Len(); i++) {
		TStr Key = KeyV[i];
		TSpDmozCluster& Child = ChildH.GetDat(Key);
		Sum += CountDescendants(Child);
	}

	Root.NItems = Sum;
	return Sum;
}

void TSpDmozClustUtils::ToSpClustV(const TVec<TSpDmozCluster>& DmozClustV, TSpClusterV& ClusterV) const {
	//create a hash of <name, idx> pairs, to avoid O(n^2)
	THash<TStr, int> NameIdxH;
	for (int i = 0; i < DmozClustV.Len(); i++)
		NameIdxH.AddDat(DmozClustV[i].Name, i);

	// traverse through the cluster vector again, creating a list of
	// indexes for each cluster
	for (int i = 0; i < DmozClustV.Len(); i++) {
		const TSpDmozCluster& Clust = DmozClustV[i];

		TVec<TStr> ChildNameV;	Clust.ChildH.GetKeyV(ChildNameV);
		TVec<int> ChildIdxV;

		for (int j = 0; j < ChildNameV.Len(); j++)
			ChildIdxV.Add(NameIdxH(ChildNameV[j]));

		TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtPrKdV;
		TSpCluster SpClust(TUInt64StrKd((uint64) 0, Clust.Name), RecIdKwWgtPrKdV, Clust.Position, Clust.Size);
		SpClust.ChildIdxV = ChildIdxV;
		
		ClusterV.Add(SpClust);
	}
}

void TSpDmozClustUtils::TSpDmozCluster::PruneNotIn(const THashSet<TStr>& ClustSet) {
	TStrV ChildKeyV;	ChildH.GetKeyV(ChildKeyV);
	for (int i = 0; i < ChildKeyV.Len(); i++) {
		const TStr& Key = ChildKeyV[i];
		TSpDmozCluster& Cluster = ChildH(Key);

		if (!ClustSet.IsKey(Cluster.Path)) {
			// remove the cluster
			ChildH.DelIfKey(Key);
		} else {
			Cluster.PruneNotIn(ClustSet);
		}
	}
}

void TSpDmozClustUtils::TSpDmozCluster::ToVector(TVec<TSpDmozCluster>& Vector) const {
	Vector.Add(*this);
	TVec<TSpDmozCluster> Children;	this->ChildH.GetDatV(Children);
	for (int i = 0; i < Children.Len(); i++) {
		Children[i].ToVector(Vector);
	}
}

TSpDmozClustUtils::TSpDmozClustUtils(const TStr& DmozFilePath):
		DMozCfy(*TFIn::New(DmozFilePath)),
		CSec() {}

void TSpDmozClustUtils::CalcClusters(const TSpItemV& ItemV,
        TSpClusterV& ClusterV, TFltVV& DocClustSimVV, bool&) {

	TVec<TStr> CategoriesAllV;
	THash<TStr, THash<TInt, TSpItem>> CategoryItemH;

	TSpDmozCluster Root;
	Root.NItems = 0;
	{
		TLock Lck(CSec);

		//classifying with DMOZ and creating a tree
		for (int i = 0; i < ItemV.Len(); i++) {
			const TSpItem& Item = ItemV[i];
			TStr Url = Item.Url;

			TStrFltKdV CatNmWgtV, KeyWdWgtV;
			DMozCfy.Classify(Url, CatNmWgtV, KeyWdWgtV, 1);

			TStr CatPath = CatNmWgtV[0].Key;

			//now that I have got the category, I need to make an entry into the tree
			TVec<TStr> Categories;
			TChA CurrentPath = "";
			CatPath.SplitOnAllAnyCh("/", Categories);
			TSpDmozCluster* PCurrNode = &Root;
			for (int CatIdx = 0; CatIdx < Categories.Len(); CatIdx++) {
				TStr Cat = Categories[CatIdx];
				CurrentPath += "/" + Cat;
				THash<TStr, TSpDmozCluster>& ChildH = PCurrNode->ChildH;

				if(!ChildH.IsKey(Cat)) {
					TSpDmozCluster Child;
					Child.Name = Cat;
					Child.Path = CurrentPath;
					Child.NItems = 0;
					ChildH.AddDat(Cat, Child);
				}

				//if the node is a leaf => add 1 to the number of items
				if (CatIdx == Categories.Len() - 1)
					ChildH(Cat).NItems++;

				//update the CategoryItemHash
				if (!CategoryItemH.IsKey(Cat)) {
					THash<TInt, TSpItem> ItemH;
					CategoryItemH.AddDat(Cat, ItemH);
				}
				CategoryItemH(Cat).AddDat(Item.Id, Item);

				PCurrNode = &ChildH(Cat);
			}
		}
	}

	//don't let the root have only 1 child
	while (Root.ChildH.Len() == 1) {
		TVec<TSpDmozCluster> ChildV;
		Root.ChildH.GetDatV(ChildV);
		Root = ChildV[0];
	}

	int NItems = ItemV.Len();
	CountDescendants(Root);
	int NClusters = Prune(Root);
	if (NClusters > TSpDmozClustUtils::MaxClusters) {
		// put all the clusters in a list, truncate that list to size: TSpDmozClustUtils::MaxClusters
		// leave only the clusters in that list
		TVec<TSpDmozCluster> AllClusts;	Root.ToVector(AllClusts);

		// sort & truncate
		AllClusts.SortCmp(TSpDmozCluster::TSpDmozClustCmp());
		AllClusts.Trunc(TSpDmozClustUtils::MaxClusters);

		THashSet<TStr> ClustSet;
		for (int i = 0; i < AllClusts.Len(); i++) {
			ClustSet.AddKey(AllClusts[i].Path);
		}
		Root.PruneNotIn(ClustSet);
	}
	CalcPositions(Root, NItems);

	TVec<TSpDmozCluster> DmozClustV;	Root.ToVector(DmozClustV);
	ToSpClustV(DmozClustV, ClusterV);

	//calculating similarities
    DocClustSimVV.Gen(ItemV.Len(), ClusterV.Len());

	for (int ItemN = 0; ItemN < ItemV.Len(); ItemN++) {
		const TSpItem& Item = ItemV[ItemN];

		TVec<double> SimV;
		for (int ClusterN = 0; ClusterN < ClusterV.Len(); ClusterN++) {
			THash<TInt, TSpItem> ItemsInCat = CategoryItemH(ClusterV[ClusterN].RecIdTopKwKd.Dat);
            if (ItemsInCat.IsKey(Item.Id)) {
                DocClustSimVV(ItemN, ClusterN) = 1.0;
            }
		}
	}
}

int counter = 0;

///////////////////////////////////////////////
// SearchPoint

void TSpSearchPoint::GenClusters(const TStr& WidgetKey, const TSpItemV& ItemV,
        TSpClusterV& ClustV, TFltVV& DocClustSimVV, bool& HasBgClust) {
	//get the clustering utils out of the hash
    EAssert(ClustUtilsH.IsKey(WidgetKey));
	TSpClustUtils& ClustUtils = *ClustUtilsH.GetDat(WidgetKey);

	// add clusters to results and calc similarities
    ClustUtils.CalcClusters(ItemV, ClustV, DocClustSimVV, HasBgClust);
}

//////////////////////////////
/// SearchPoint - default implementation
TSpSearchPointImpl::TSpSearchPointImpl(
            const TClustUtilH& _ClustUtilsH,
            const TStr& _DefaultClustUtilsKey,
            const int& _PerPage,
            const PNotify& Notify):
        TSpSearchPoint(_ClustUtilsH, _DefaultClustUtilsKey, _PerPage, Notify) {}


/* PSpSearchPoint TSpSearchPointImpl::New(PSpClustUtils& PClustUtils, const int& PerPage, const PSpDataSource& DataSource, */
/* 			const PNotify& Notify) { */
/* 	THash<TStr, PSpClustUtils> UtilsH; */
/* 	TStr DefaultKey = "default"; */
/* 	UtilsH.AddDat(DefaultKey, PClustUtils); */
/* 	return TSpSearchPointImpl::New(UtilsH, DefaultKey, PerPage, DataSource, Notify); */
/* } */

/* PSpSearchPoint TSpSearchPointImpl::New(THash<TStr, PSpClustUtils>& ClustUtilsH, TStr& DefaultUtilsKey, const int& PerPage, */
/* 		const PSpDataSource& DataSource, const PNotify& Notify) { */
/* 	return new TSpSearchPointImpl(ClustUtilsH, DefaultUtilsKey, PerPage, DataSource, Notify); */
/* } */

bool TSpSearchPointImpl::KwSuitable(const TStr& Kw, const TStrV& KwV) {
	for (int i = 0; i < KwV.Len(); i++) {
		const TStr& Kw1 = KwV[i];

		if (Kw1.IsStrIn(Kw) || Kw.IsStrIn(Kw1)) return false;
	}

	return true;
}

void TSpSearchPointImpl::GetResultsByWSim(const TFltPrV& PosV, const TSpClusterV& ClustV, 
            const TFltVV& ItemClustSimVV, const bool& HasBgClust, const int& Offset, const int& Limit, TIntV& PermuteV) const {
    if (!PermuteV.Empty()) { PermuteV.Clr(); }

    // TODO check why this was here
    /* if (!HasClusters(ClusterV)) { */
    /*     return; */
    /* } */

	TFltPr Pos = PosV[0];
	double x = Pos.Val1;
	double y = Pos.Val2;

    const int NItems = ItemClustSimVV.GetRows();
	const int NClusts = HasBgClust ? ClustV.Len() - 1 : ClustV.Len();

	// calculate distance to each cluster
	TFltV DistV;
	for (int i = 0; i < NClusts; i++) {
		const TSpCluster& Clust = ClustV[i];
		const double Dist = TMath::Mx(sqrt(pow(x - Clust.Pos.Val1, 2.0) + pow(y - Clust.Pos.Val2, 2.0)), TSpSearchPointImpl::MinDist);
		DistV.Add(Dist);
	}

	double CenterX = TSpSearchPoint::MaxClusterX/2;
	double CenterY = TSpSearchPoint::MaxClusterY/2;

	// for each item calculate a weighted sum of the similarity to each cluster
	// the weight will be 1/(dist to the cluster)
 	THash<TInt, double> ScoreH;
	for (int ItemN = 0; ItemN < NItems; ItemN++) {
		// calc the sum
		double Sum = 0;
		for (int ClustN = 0; ClustN < NClusts; ClustN++) {
			const double& DocClustSim = ItemClustSimVV(ItemN, ClustN);
			const double& Dist = DistV[ClustN];
			Sum += DocClustSim/Dist;
		}

		// add a hidden cluster in the center, so that the default rank is taken into account
		double CenterW = (NItems - ItemN)/(TMath::Mx(pow(CenterX - x, 2.0) + pow(CenterY - y, 2.0),
			TSpSearchPointImpl::MinDist)*NItems);
		Sum += CenterW;

		ScoreH.AddDat(ItemN, Sum);
	}

	ScoreH.SortByDat(false);

	TVec<TPair<TInt, double>> KeyDatPrV;		ScoreH.GetKeyDatPrV(KeyDatPrV);
	for (int i = Offset; i < Offset + Limit && i < KeyDatPrV.Len(); i++) {
		const TPair<TInt, double>& HKeyDat = KeyDatPrV[i];
		const int ItemN = HKeyDat.Val1;

		PermuteV.Add(ItemN);
	}
}


void TSpSearchPointImpl::GetKwsByWSim(const TFltPr& PosPr, const TSpClusterV& ClustV,
       const bool& HasBgClust, TStrV& KwV) const {
	const int NClusts = HasBgClust ? ClustV.Len() - 1 : ClustV.Len();

	// calculate the weight of each cluster
	TFltV ClustWgtV(ClustV.Len(), 0);
	for (int ClustIdx = 0; ClustIdx < NClusts; ClustIdx++) {
		const TSpCluster& Clust = ClustV[ClustIdx];

		const double Dist = TMath::Mx(sqrt(pow(PosPr.Val1 - Clust.Pos.Val1, 2.0) + pow(PosPr.Val2 - Clust.Pos.Val2, 2.0)), TSpSearchPointImpl::MinDist);
		ClustWgtV.Add(1 / Dist);
	}

	int MaxKws = 20;
	
	// normalize the distances to get weights
	TLinAlg::Normalize(ClustWgtV);

	// compute keyword weights
	TStrFltKdV KwStrWgtPrV;
	for (int ClustIdx = 0; ClustIdx < NClusts; ClustIdx++) {
		const TSpCluster& Clust = ClustV[ClustIdx];
		const double ClustWgt = ClustWgtV[ClustIdx];

		for (int KwIdx = 0; KwIdx < Clust.RecIdKwWgtFqTrKdV.Len(); KwIdx++) {
			const TStrFltFltTr& KwStrWgtFqTr =  Clust.RecIdKwWgtFqTrKdV[KwIdx].Dat;

			KwStrWgtPrV.Add(TStrFltKd(KwStrWgtFqTr.Val1, KwStrWgtFqTr.Val2 * ClustWgt));
		}
	}

	KwStrWgtPrV.SortCmp(TCmpKeyDatByDat<TStr, TFlt>());

	const int NKws = TMath::Mn(KwStrWgtPrV.Len(), MaxKws);

	KwV.Clr();
	for (int KwIdx = KwStrWgtPrV.Len() - 1; KwIdx >= KwStrWgtPrV.Len() - NKws; KwIdx--) {
		const TStr& Kw = KwStrWgtPrV[KwIdx].Key;
		if (TSpSearchPointImpl::KwSuitable(Kw, KwV))
			KwV.Add(KwStrWgtPrV[KwIdx].Key.ToLc());
	}
}

PJsonVal TSpSearchPoint::GenItemsJSon(const TSpItemV& ItemV, const int& Offset, const int& Limit) const {
	//create a JSON array of items
	PJsonVal JSonArr = TJsonVal::NewArr();

	for (int i = Offset; i < Offset + Limit && i < ItemV.Len(); i++) {
		const TSpItem& Item = ItemV[i];
		PJsonVal JSonObj = TJsonVal::NewObj();

        const PJsonVal TitleJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.Title));
        const PJsonVal DescJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.Description));
        const PJsonVal DispUrlJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.DisplayUrl));
        const PJsonVal UrlJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.Url));

        EAssertR(TitleJson->IsStr(), "Title is not string: " + Item.Title + "!");
        EAssertR(DescJson->IsStr(), "Description is not string: " + Item.Description + "!");
        EAssertR(TitleJson->IsStr(), "Display URL is not string: " + Item.DisplayUrl + "!");
        EAssertR(TitleJson->IsStr(), "URL is not string: " + Item.Url + "!");

		JSonObj->AddToObj("title", TitleJson);
		JSonObj->AddToObj("description", DescJson);
		JSonObj->AddToObj("displayURL", DispUrlJson);
		JSonObj->AddToObj("URL", UrlJson);
		JSonObj->AddToObj("rank", TJsonVal::NewNum(Item.Id));

		JSonArr->AddToArr(JSonObj);
	}

	return JSonArr;
}

TSpSearchPoint::TSpSearchPoint(
            const THash<TStr, TSpClustUtils*>& _ClustUtilsH,
            const TStr& _DefaultClustUtilsKey,
            const int& _PerPage,
            const PNotify& _Notify):
		ClustUtilsH(_ClustUtilsH),
		DefaultClustUtilsKey(_DefaultClustUtilsKey),
		PerPage(_PerPage),
		CacheSection(),
		Notify(_Notify) {}

PJsonVal TSpSearchPoint::CreateClusterJSon(const TSpCluster& Cluster) const {
	// keywords
	
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

	PJsonVal JSonObj = TJsonVal::NewObj();

    const PJsonVal TextJson = TJsonVal::NewStr(THtmlLx::GetEscapedStr(Cluster.RecIdTopKwKd.Dat));

    EAssertR(TextJson->IsStr(), "Cluster text JSON is not string!");

	JSonObj->AddToObj("text", TextJson);
	JSonObj->AddToObj("kwords", KwsJson);
	JSonObj->AddToObj("x", TJsonVal::NewNum(Cluster.Pos.Val1));
	JSonObj->AddToObj("y", TJsonVal::NewNum(Cluster.Pos.Val2));
	JSonObj->AddToObj("size", TJsonVal::NewNum(Cluster.Size));
	JSonObj->AddToObj("childIdxs", ChildIdxArr);

	return JSonObj;
}

PJsonVal TSpSearchPoint::GenClustJSon(const THash<TStr, TSpClusterV>& ClustVH) const {
	//create a JSON array of clusters
	TVec<TSpClusterV> ClusterVV;  ClustVH.GetDatV(ClusterVV);

	PJsonVal Result = TJsonVal::NewArr();

	for (int i = 0; i < ClusterVV.Len(); i++) {
		const TSpClusterV& ClusterV = ClusterVV[i];
		PJsonVal JSonArr = TJsonVal::NewArr();
		for (int j = 0; j < ClusterV.Len(); j++) {
			const TSpCluster& Cluster = ClusterV[j];

			PJsonVal ClustJSon = CreateClusterJSon(Cluster);
			JSonArr->AddToArr(ClustJSon);
		}

		Result->AddToArr(JSonArr);
	}

	return Result;
}

PJsonVal TSpSearchPoint::GenJson(const TSpResult& SpResult, const int& Offset, const int& Limit, const bool& SummaryP) const {
	PJsonVal Result = TJsonVal::NewObj();

    const TStr QueryId = SpResult.QueryId;

    EAssertR(!QueryId.Empty(), "The query ID is empty!");

	Result->AddToObj("items", GenItemsJSon(SpResult.ItemV, Offset, Limit));
	Result->AddToObj("totalItems", TJsonVal::NewNum(SpResult.ItemV.Len()));
	Result->AddToObj("clusters", SpResult.HasClusters() ?
                                    GenClustJSon(SpResult.ClusterVH) : TJsonVal::NewArr());

	return Result;
}

///////////////////////////////////////////////
// Processes a request to update position
void TSpSearchPoint::ProcPosPageRq(const TFltPrV& PosV, const TSpClusterV& ClusterV,
        const TFltVV& ItemClustSimVV, const bool& HasBgClust, const int& Page, TIntV& PermuteV) {
	// get all the needed objects out of the Query
	/* TSpResult& SpResult = QueryHandle.GetQuery().GetResult(); */

    // TODO check why this was here
	/* if (!SpResult.HasClusters()) { */
	/* 	return TJsonVal::NewStr("\"NOT_ENOUGH_DATA\""); */
	/* } */

	// calculate the offset and limit
	int Offset = PerPage * Page;
	int Limit = PerPage;

	TSpItemV ItemV;	GetResultsByWSim(PosV, ClusterV, ItemClustSimVV, HasBgClust, Offset, Limit, PermuteV);
}

PJsonVal TSpSearchPoint::ProcessPosKwRq(const TFltPr& Pos, const TSpClusterV& ClustV,
        const bool& HasBgClust) {
    /* TSpResult& Result = QueryHandle.GetQuery().GetResult(); */
	/* if (!Result.HasClusters()) { */
		/* return TJsonVal::NewStr("\"NOT_ENOUGH_DATA\""); */
	/* } */

	TStrV KwV;	GetKwsByWSim(Pos, ClustV, HasBgClust, KwV);

	// convert to JSON
	PJsonVal KwsJson = TJsonVal::NewArr();
	for (int KwIdx = 0; KwIdx < KwV.Len(); KwIdx++) {
		KwsJson->AddToArr(TJsonVal::NewStr(KwV[KwIdx]));
	}

	return KwsJson;
}
