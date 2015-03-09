#include "sp.h"

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

const int TSpBingEngine::MaxResultsPerQuery = 50;

const TUInt64 TSpQueryManager::MaxIdleSecs = 1800;	// == 60*30 == 30min

const double TSpSearchPoint::MinDist = 1e-11;


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
// SearchPoint - Data Source
TSpDataSource::TSpDataSource(const PNotify& _Notify):
		Notify(_Notify) {}

///////////////////////////////////////////////
// SearchPoint-Bing-Search-Engine
TSpBingEngine::TSpBingEngine(const TStrV& _BingApiKeyV, const PNotify& Notify):
		TSpDataSource(Notify),
		BingApiKeyV(_BingApiKeyV),
		Rnd(0) {}

int TSpBingEngine::ParseResponseBody(const TStr& XmlData, TSpItemV& ResultItemV, const int& Offset) const {
	PXmlDoc PDoc = TXmlDoc::LoadStr(XmlData);

	EAssertR(PDoc->IsOk(), "Invalid response from search API: " + XmlData);

	TXmlTokV ResultTokV;	PDoc->GetTagTokV("feed|entry", ResultTokV);
	//iterate over all the results parse each result and add it to the result vector
	for (int i = 0; i < ResultTokV.Len(); i++) {
		PXmlTok PResultTok = ResultTokV[i];
		PXmlTok PropsTok = PResultTok->GetTagTok("content|m:properties");
		
		TStrStrH TagH;
		for (int j = 0; j < PropsTok->GetSubToks(); j++) {
			PXmlTok Tok = PropsTok->GetSubTok(j);

			TStr TagName = Tok->GetTagNm();
			TStr Val = Tok->GetTagVal(TagName, false);

			TagH.AddDat(TagName, Val);
		}

		ResultItemV.Add(
			TSpItem(Offset + i + 1, TagH.GetDat("d:Title"), TagH("d:Description"), TagH("d:Url"), TagH("d:DisplayUrl"))
		);
	}

	return ResultTokV.Len();
}

void TSpBingEngine::ExecuteQuery(PSpResult& SpResult, const int NResults) {
	try {
		const int NQueries = (int) ceil((double) NResults/TSpBingEngine::MaxResultsPerQuery);

		int Limit, StepFetched;

		int Offset = 0;
		int Remain = NResults;

		int QueryN = 0;
		do {
			Limit = TMath::Mn(Remain, TSpBingEngine::MaxResultsPerQuery);

			StepFetched = FetchResults(SpResult->QueryStr, SpResult->ItemV, Limit, Offset);

			Offset += StepFetched;
			Remain -= StepFetched;
		} while (StepFetched >= Limit && ++QueryN < NQueries);
	} catch (const PExcept& e) {
		Notify->OnNotifyFmt(TNotifyType::ntErr, "An exception occurred while fetching results from the document source. Nr. %s\n", e->GetMsgStr().CStr());
		throw e;
	}
}

int TSpBingEngine::FetchResults(const TStr& Query, TSpItemV& ResultItemV, const int& Limit, const int& Offset) {
	int NFetched = 0;

	try {
		// select the API key
		const TStr& ApiKey = BingApiKeyV[Rnd.GetUniDevInt(BingApiKeyV.Len())];

		// fetch the data from the server
		FILE* Pipe = OpenPipe(ApiKey, Query, Offset, Limit, Notify);

		// read the response
		TChA Resp = "";
		char Buff[128];
		while (!feof(Pipe)) {
			if (fgets(Buff, 128, Pipe) != NULL) {
				Resp += Buff;
			}
		}

		ClosePipe(Pipe);

		NFetched = TSpBingEngine::ParseResponseBody(Resp, ResultItemV, Offset);
	} catch (const PExcept& Except) {
		Notify->OnNotifyFmt(TNotifyType::ntInfo, "Failed to parse BING data: %s!", Except->GetMsgStr().CStr());
		throw Except;
	}

	return NFetched;
}

FILE* TSpBingEngine::OpenPipe(const TStr& ApiKey, const TStr& Query,
		const int& Offset, const int& Limit, const PNotify& Notify) {
	const TStr UrlStr = "https://api.datamarket.azure.com/Data.ashx/Bing/SearchWeb/v1/Web?Query=%27" + Query + "%27&\\$skip=" + TInt::GetStr(Offset) + "&\\$top=" + TInt::GetStr(Limit) + "&\\$format=Atom";
	const TStr FetchStr = "curl -k -H \"Authorization: Basic " + ApiKey + "\" \"" + TUrl::New(UrlStr)->GetUrlStr() + "\"";

	Notify->OnNotifyFmt(TNotifyType::ntInfo, "Fetching: %s", FetchStr.CStr());

#ifdef GLib_WIN
	FILE* Pipe = _popen(FetchStr.CStr(), "r");
#else
	FILE* Pipe = popen(FetchStr.CStr(), "r");
#endif

	EAssertR(Pipe != NULL, "Failed to run curl command!");
	return Pipe;
}

void TSpBingEngine::ClosePipe(FILE* Pipe) {
#ifdef GLib_WIN
		_pclose(Pipe);
#else
		pclose(Pipe);
#endif
}

void TSpBingEngine::CreateTagNameHash(THash<TStr, TChA>& TagHash) const {
	TagHash.AddDat("web:Title", "");
	TagHash.AddDat("web:Description", "");
	TagHash.AddDat("web:Url", "");
	TagHash.AddDat("web:CacheUrl", "");
	TagHash.AddDat("web:DisplayUrl", "");
	TagHash.AddDat("web:DateTime", "");
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

void TSpKMeansClustUtils::CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV) {
	printf("Computing cluters...\n");

	const TSpItemV& ItemV = SpResult->ItemV;
	THash<TInt, TVec<double>>& SimsH =  SpResult->DocIdClustSimH;

	SimsH.Clr();

	if (ItemV.Len() < 3*TSpKMeansClustUtils::MinItemsPerCluster)	// check if enough clusters
		return;

	int Clusts = TMath::Mn((int) ceil((double) ItemV.Len() / TSpKMeansClustUtils::MinItemsPerCluster), TSpKMeansClustUtils::NClusters);

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

	THash<TInt, TIntV> MergedH;	JoinClusters(DocPointV, MergedH, TSpKMeansClustUtils::MaxJoinDist);
	TIntV TopClustIdxV;	MergedH.GetKeyV(TopClustIdxV);

	for (int i = 0; i < TopClustIdxV.Len(); i++) {
		const int& TopClustIdx = TopClustIdxV[i];
		TIntV& ClustsIdxs = MergedH.GetDat(TopClustIdx);
		ClustsIdxs.Ins(0, TopClustIdx);

		// compute the avg coordinates
		const int NClusts = ClustsIdxs.Len();
		TFltPr AvgCoords(0, 0);
		for (int j = 0; j < NClusts; j++) {
			const int& ClustIdx = ClustsIdxs[j];
			const TFltV& ClustPos = DocPointV[ClustIdx];

			AvgCoords.Val1 += ClustPos[0] / NClusts;
			AvgCoords.Val2 += ClustPos[1] / NClusts;
		}

		const int KwsInJointClust = (int) (2*TSpKMeansClustUtils::KwsPerClust*(1.0 - TMath::Power(2, -NClusts)));	// KwsPerClust * Sum[2^(-i), {i,0,NClusts-1}]
		const int NKwsDefault = KwsInJointClust / NClusts;

		// get the keywords
		TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtFqTrKdV(NKwsDefault * NClusts, 0);
		int CurrClustId = 0;
		for (int j = 0; j < NClusts; j++) {
			const int ClustIdx = ClustsIdxs[j];
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

		TSpCluster Cluster(TUInt64StrKd((uint64) i, RecIdKwWgtFqTrKdV[0].Dat.Val1), RecIdKwWgtFqTrKdV,
			TFltPr(AvgCoords.Val1, AvgCoords.Val2), Size);
		ClusterV.Add(Cluster);

		// compute similarities
		for (int j = 0; j < NClusts; j++) {
			const int& ClustIdx = ClustsIdxs[j];
			PBowDocPartClust Clust = BowDocPart->GetClust(ClustIdx);
			PBowSpV ClustSpV = Clust->GetConceptSpV();

			// for each document calculate the similarity to this cluster
			for (int k = 0; k < BowDocWgtBs->GetDocs(); k++) {
				const int& DocId = BowDocWgtBs->GetDId(k);
				const int& SnippetN = BowDocBs->GetDocNm(DocId).GetInt();

				const TSpItem& Doc = ItemV[SnippetN];
				if (!SimsH.IsKey(Doc.Id)) {
					TVec<double> SimsV(TopClustIdxV.Len(), 0);

					for (int h = 0; h < TopClustIdxV.Len(); h++) {
						SimsV.Add(0);
					}

					SimsH.AddDat(Doc.Id, SimsV);
				}
				TVec<double>& DocSimV = SimsH.GetDat(Doc.Id);

				PBowSpV DocSpV = BowDocWgtBs->GetSpV(DocId);
				TFlt ClustDocSim = BowSim->GetSim(ClustSpV, DocSpV);

				DocSimV[i] += ClustDocSim / NClusts;
			}
		}
	}

	printf("Finished!\n");
}

///////////////////////////////////////////////
// SearchPoint DP-Means Clustering Utilities
void TSpDPMeansClustUtils::CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV) {
	Notify->OnNotify(TNotifyType::ntInfo, "Computing clusters...");

	const TSpItemV& ItemV = SpResult->ItemV;
	THash<TInt, TVec<double>>& SimsH =  SpResult->DocIdClustSimH;

	if (ItemV.Empty()) { return; }

	SimsH.Clr();

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

	Notify->OnNotify(TNotifyType::ntInfo, "Clusters computed, computing statistics ...");

	// iterate over discovered clusters
    for (int i = 0; i < BowDocPart->GetClusts(); i++) {
        PBowDocPartClust Clust = BowDocPart->GetClust(i);
        PBowSpV ClustSpV = Clust->GetConceptSpV();

        //for each document calculate the similarity to this cluster
        for (int j = 0; j < BowDocWgtBs->GetDocs(); j++) {
            int DocId = BowDocWgtBs->GetDId(j);
            int SnippetN = BowDocBs->GetDocNm(DocId).GetInt();

            const TSpItem& Doc = ItemV[SnippetN];

            //if the similarities aren't in the hash yet => put them there
            if (!SimsH.IsKey(Doc.Id)) {
                TVec<double> SimsV;
                SimsH.AddDat(Doc.Id, SimsV);
            }
            TVec<double>& DocSimV = SimsH(Doc.Id);

            PBowSpV DocSpV = BowDocWgtBs->GetSpV(DocId);
            TFlt ClustDocSim = BowSim->GetSim(ClustSpV, DocSpV);

            DocSimV.Add(ClustDocSim);
        }
    }

    Notify->OnNotify(TNotifyType::ntInfo, "Transforming onto a plane ...");

    //transforming the clusters onto a 2D space
    int NClusts = BowDocPart->GetClusts();
	
	// put the clusters in a circle
	TVec<TFltV> DocPointV(NClusts+1,0);
	double ThetaStart = TRnd(0).GetUniDev()*TMath::Pi*2;
	double ThetaStep = 2*TMath::Pi / NClusts;
	for (int i = 0; i < NClusts; i++) {
		const double Theta = ThetaStart + i*ThetaStep;
		
		TFltV PosV;
		PosV.Add(cos(Theta));
		PosV.Add(sin(Theta));

		DocPointV.Add(PosV);
	}

	TFltV PosV;
	PosV.Add(0);
	PosV.Add(0);
	DocPointV.Add(PosV);

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
	TVizMapFactory::MakeFlat(ClustSet, TVizDistType::vdtCos, DocPointV, 10000, 1, TMath::Pi / (2*NClusts), false, Notify);

    DocPointV.Del(DocPointV.Len()-1);

    Notify->OnNotify(TNotifyType::ntInfo, "Transforming to [0,1]x[0,1] ...");
    //transforming to the canvas interval
    TSpUtils::TransformInterval(DocPointV, 0.0, TSpSearchPoint::MaxClusterX,
		0.0, TSpSearchPoint::MaxClusterY);

    //OK, now I've got the points, I just need to connect them back to the clusters
    for (int i = 0; i < DocPointV.Len(); i++) {
        TFltV Point = DocPointV[i];
        PBowDocPartClust BowDocPartClust = BowDocPart->GetClust(i);

        // get keywords
		PBowSpV ClustSpV = BowDocPartClust->GetConceptSpV();
        PBowKWordSet ClustKWordSet = ClustSpV->GetKWordSet(BowDocBs)->GetTopKWords(
			TSpDPMeansClustUtils::KwsPerClust, 1.0);
        
		const int NKws = TMath::Mn(ClustKWordSet->GetKWords(), TSpDPMeansClustUtils::KwsPerClust);
		if (NKws > 1) {
			TStr TopKeyWord = ClustKWordSet->GetKWordStr(0);

		
			TVec<TKeyDat<TUInt64, TStrFltFltTr>> RecIdKwWgtFqTrKdV(NKws, 0);
			TStrV KwV;
        
			for (int j = 0; j < NKws; j++) {
				const TStr Kw = ClustKWordSet->GetKWordStr(j);

				if (!TSpSearchPointImpl::KwSuitable(Kw, KwV)) continue;

				const TFlt KwWgt = ClustKWordSet->GetKWordWgt(j);
				const TFlt GlobalKWordFq = BowDocWgtBs->GetWordFq(BowDocBs->GetWId(Kw));

				RecIdKwWgtFqTrKdV.Add(TKeyDat<TUInt64, TStrFltFltTr>(
						(uint64) i, TStrFltFltTr(Kw, KwWgt, GlobalKWordFq)));
				KwV.Add(Kw);
			}

			TSpCluster Cluster(TUInt64StrKd((uint64) i, RecIdKwWgtFqTrKdV[0].Dat.Val1), RecIdKwWgtFqTrKdV, TFltPr(Point[0], Point[1]),
					TSpClustUtils::MAX_CLUST_RADIUS);
			ClusterV.Add(Cluster);
		}
    }

	// create the central cluster
//	const TFltV& WordWFqV = BowDocWgtBs->GetWordFqV();

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

	TSpCluster Cluster(TUInt64StrKd((uint64) DocPointV.Len(), RecIdKwWgtFqTrKdV[0].Dat.Val1), RecIdKwWgtFqTrKdV, TFltPr(.5, .5),
			TSpClustUtils::MAX_CLUST_RADIUS);
	ClusterV.Add(Cluster);
	SpResult->HasBackgroundClust = true;

	Notify->OnNotify(TNotifyType::ntInfo, "Finished!");
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

void TSpDmozClustUtils::CalcClusters(const PSpResult& SpResult, TSpClusterV& ClusterV) {
	const TSpItemV& ResultItemV = SpResult->ItemV;
	
	TVec<TStr> CategoriesAllV;
	THash<TStr, THash<TInt, TSpItem>> CategoryItemH;

	TSpDmozCluster Root;
	Root.NItems = 0;
	{
		TLock Lck(CSec);

		//classifying with DMOZ and creating a tree
		for (int i = 0; i < ResultItemV.Len(); i++) {
			const TSpItem& Item = ResultItemV[i];
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

	int NItems = ResultItemV.Len();
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
	for (int i = 0; i < ResultItemV.Len(); i++) {
		const TSpItem& Item = ResultItemV[i];

		TVec<double> SimV;
		for (int j = 0; j < ClusterV.Len(); j++) {
			THash<TInt, TSpItem> ItemsInCat = CategoryItemH(ClusterV[j].RecIdTopKwKd.Dat);
			if (ItemsInCat.IsKey(ResultItemV[i].Id))
				SimV.Add(1.0);
			else
				SimV.Add(0.0);
		}
		SpResult->DocIdClustSimH.AddDat(Item.Id, SimV);
	}
}

int counter = 0;

///////////////////////////////////////////////
// SearchPoint
PSpResult TSpSearchPoint::GetCachedResult(const TStr& QueryId) {
	TLock Lck(CacheSection);
	return IsResultCached(QueryId) ? PQueryManager->GetQuery(QueryId)->PResult : NULL;
}

void TSpSearchPoint::CacheResult(const PSpResult& Result) {
	TLock Lck(CacheSection);
	PQueryManager->NewQuery(Result);
}

PSpResult TSpSearchPoint::GenClusters(PSpResult& SpResult, const TStr& _ClustUtilsKey) {
	{
		TLock Lck(CacheSection);
		if (PQueryManager->IsQuery(SpResult->QueryId))
			return GetCachedResult(SpResult->QueryId);
	}
	//get the clustering utils out of the hash
	TStr ClustUtilsKey = ClustUtilsH.IsKey(_ClustUtilsKey) ? _ClustUtilsKey : DefaultClustUtilsKey;
	PSpClustUtils PClustUtils = ClustUtilsH.GetDat(ClustUtilsKey);

	// add clusters to results and calc similarities
	TSpClusterV ClusterV;	PClustUtils->CalcClusters(SpResult, ClusterV);
	SpResult->ClusterVH.AddDat("default", ClusterV);

	CacheResult(SpResult);

	return SpResult;
}


void TSpSearchPoint::GetResultsByWSim(const TFltPrV& PosV, const TStr& QueryId, TSpItemV& ResultV, const int& Offset, const int& Limit) {
	PSpQuery PQuery = NULL;
	{
		TLock Lck(CacheSection);
		if (!PQueryManager->IsQuery(QueryId))
				return;
		PQuery = PQueryManager->GetQuery(QueryId);
	}

	// get all the needed objects out of the Query
	PSpResult PResultObj = PQuery->PResult;
	if (!PResultObj->HasClusters())
		return;

	GetResultsByWSim(PosV, PResultObj, ResultV, Offset, Limit);
}

bool TSpSearchPoint::IsResultCached(const TStr& QueryId) {
	TLock Lck(CacheSection);
	return PQueryManager->IsQuery(QueryId);
}

TSpSearchPointImpl::TSpSearchPointImpl(THash<TStr, PSpClustUtils>& _ClustUtilsH, TStr& _DefaultClustUtilsKey, const int& _PerPage,
		const PSpDataSource& DataSource, const PNotify& Notify):
		TSpSearchPoint(_ClustUtilsH, _DefaultClustUtilsKey, _PerPage, DataSource, TSpQueryManager::New(), Notify) {}


PSpSearchPoint TSpSearchPointImpl::New(PSpClustUtils& PClustUtils, const int& PerPage, const PSpDataSource& DataSource,
			const PNotify& Notify) {
	THash<TStr, PSpClustUtils> UtilsH;
	TStr DefaultKey = "default";
	UtilsH.AddDat(DefaultKey, PClustUtils);
	return TSpSearchPointImpl::New(UtilsH, DefaultKey, PerPage, DataSource, Notify);
}

PSpSearchPoint TSpSearchPointImpl::New(THash<TStr, PSpClustUtils>& ClustUtilsH, TStr& DefaultUtilsKey, const int& PerPage,
		const PSpDataSource& DataSource, const PNotify& Notify) {
	return new TSpSearchPointImpl(ClustUtilsH, DefaultUtilsKey, PerPage, DataSource, Notify);
}

bool TSpSearchPointImpl::KwSuitable(const TStr& Kw, const TStrV& KwV) {
	for (int i = 0; i < KwV.Len(); i++) {
		const TStr& Kw1 = KwV[i];

		if (Kw1.IsStrIn(Kw) || Kw.IsStrIn(Kw1)) return false;
	}

	return true;
}

void TSpSearchPointImpl::GetResultsByWSim(const TFltPrV& PosV, const PSpResult& SpResult, TSpItemV& ResultV, const int& Offset, const int& Limit) const {
	TFltPr Pos = PosV[0];
	double x = Pos.Val1;	
	double y = Pos.Val2;

	const TSpItemV& ItemV = SpResult->ItemV;
	const TSpClusterV& ClustV = SpResult->ClusterVH.GetDat("default");
	const THash<TInt, TVec<double>>& SimsH = SpResult->DocIdClustSimH;

	const int NItems = ItemV.Len();

	const int NClusts = SpResult->HasBackgroundClust ? ClustV.Len() - 1 : ClustV.Len();

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
	for (int ItemIdx = 0; ItemIdx < NItems; ItemIdx++) {
		const TSpItem& Item = ItemV[ItemIdx];
		const TVec<double>& Sims = SimsH.GetDat(Item.Id);

		// calc the sum
		double sum = 0;
		for (int ClustIdx = 0; ClustIdx < NClusts; ClustIdx++) {
			const double& DocClustSim = Sims[ClustIdx];
			const double& Dist = DistV[ClustIdx];
			sum += DocClustSim/Dist;
		}

		// add a hidden cluster in the center, so that the default rank is taken into account
		double CenterW = (NItems - Item.Id)/(TMath::Mx(pow(CenterX - x, 2.0) + pow(CenterY - y, 2.0),
			TSpSearchPointImpl::MinDist)*ItemV.Len());
		sum += CenterW;

		ScoreH.AddDat(Item.Id, sum);
	}

	ScoreH.SortByDat(false);

	TVec<TPair<TInt, double>> KeyDatPrV;		ScoreH.GetKeyDatPrV(KeyDatPrV);
	ResultV.Clr();
	for (int i = Offset; i < Offset + Limit && i < KeyDatPrV.Len(); i++) {
		const TPair<TInt, double>& HKeyDat = KeyDatPrV[i];
		const int ItemIdx = HKeyDat.Val1 - 1;		//idx == rank - 1

		ResultV.Add(ItemV[ItemIdx]);
	}
}


void TSpSearchPointImpl::GetKwsByWSim(const TFltPr& PosPr, const PSpResult& SpResult, TStrV& KwV) const {
	const TSpClusterV& ClustV = SpResult->ClusterVH.GetDat("default");

	const int NClusts = SpResult->HasBackgroundClust ? ClustV.Len() - 1 : ClustV.Len();

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

		JSonObj->AddToObj("title", TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.Title)));
		JSonObj->AddToObj("description", TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.Description)));
		JSonObj->AddToObj("displayURL", TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.DisplayUrl)));
		JSonObj->AddToObj("URL", TJsonVal::NewStr(THtmlLx::GetEscapedStr(Item.Url)));
		JSonObj->AddToObj("rank", TJsonVal::NewNum(Item.Id));

		JSonArr->AddToArr(JSonObj);
	}

	return JSonArr;
}

TSpSearchPoint::TSpSearchPoint(const THash<TStr, PSpClustUtils>& _ClustUtilsH,
		const TStr& _DefaultClustUtilsKey, const int& _PerPage, const PSpDataSource& _DataSource,
		const PSpQueryManager& QueryManager, const PNotify& _Notify):
		PQueryManager(QueryManager),
		ClustUtilsH(_ClustUtilsH),
		DataSource(_DataSource),
		DefaultClustUtilsKey(_DefaultClustUtilsKey),
		PerPage(_PerPage),
		CacheSection(TCriticalSectionType::cstRecursive),
		Notify(_Notify) {}

PJsonVal TSpSearchPoint::CreateClusterJSon(const TSpCluster& Cluster) const {
	// keywords
	
	PJsonVal KwsJson = TJsonVal::NewArr();
	for (int j = 0; j < Cluster.RecIdKwWgtFqTrKdV.Len(); j++) {
		const TStrFltFltTr KwWgtPr = Cluster.RecIdKwWgtFqTrKdV[j].Dat;

		PJsonVal KwJson = TJsonVal::NewObj();
		KwJson->AddToObj("text", TJsonVal::NewStr(THtmlLx::GetEscapedStr(KwWgtPr.Val1)));
		KwJson->AddToObj("weight", TJsonVal::NewNum(KwWgtPr.Val2));
		KwJson->AddToObj("fq", TJsonVal::NewNum(KwWgtPr.Val3));

		KwsJson->AddToArr(KwJson);
	}

	//children
	PJsonVal ChildIdxArr = TJsonVal::NewArr();
	for (int j = 0; j < Cluster.ChildIdxV.Len(); j++)
		ChildIdxArr->AddToArr(TJsonVal::NewNum(Cluster.ChildIdxV[j]));

	PJsonVal JSonObj = TJsonVal::NewObj();
	JSonObj->AddToObj("text", TJsonVal::NewStr(THtmlLx::GetEscapedStr(Cluster.RecIdTopKwKd.Dat)));
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

PJsonVal TSpSearchPoint::GenJSon(const PSpResult& SpResult, const int& Offset, const int& Limit, const bool& SummaryP) const {
	PJsonVal Result = TJsonVal::NewObj();

	Result->AddToObj("queryId", SpResult->QueryId);
	Result->AddToObj("items", GenItemsJSon(SpResult->ItemV, Offset, Limit));
	Result->AddToObj("totalItems", TJsonVal::NewNum(SpResult->ItemV.Len()));
	Result->AddToObj("clusters", SpResult->HasClusters() ?
    GenClustJSon(SpResult->ClusterVH) : TJsonVal::NewArr());

	return Result;
}

///////////////////////////////////////////////
// Processes a request to update position
PJsonVal TSpSearchPoint::ProcPosPageRq(const TFltPrV& PosV, const int& Page, const TStr& QueryId) {
	PSpQuery PQuery = NULL;
	{
		TLock Lck(CacheSection);
		PQuery = PQueryManager->GetQuery(QueryId);
	}

	// get all the needed objects out of the Query
	PSpResult PResultObj = PQuery->PResult;
	if (!PResultObj->HasClusters()) {
		return TJsonVal::NewStr("\"NOT_ENOUGH_DATA\"");
	}

	// calculate the offset and limit
	int Offset = PerPage * Page;
	int Limit = PerPage;

	TSpItemV ItemV;	GetResultsByWSim(PosV, QueryId, ItemV, Offset, Limit);

	PJsonVal ItemsJSon = GenItemsJSon(ItemV, 0, ItemV.Len());
	
	PJsonVal Result = TJsonVal::NewObj();
	Result->AddToObj("items", ItemsJSon);

	return Result;
}

PJsonVal TSpSearchPoint::ProcessPosKwRq(const TFltPr& Pos, const TStr& QueryId) {
	PSpQuery PQuery = PQueryManager->GetQuery(QueryId);

	PSpResult PResultObj = PQuery->PResult;
	if (!PResultObj->HasClusters()) {
		return TJsonVal::NewStr("\"NOT_ENOUGH_DATA\"");
	}

	TStrV KwV;	GetKwsByWSim(Pos, PResultObj, KwV);

	// convert to JSON
	PJsonVal KwsJson = TJsonVal::NewArr();
	for (int KwIdx = 0; KwIdx < KwV.Len(); KwIdx++) {
		KwsJson->AddToArr(TJsonVal::NewStr(KwV[KwIdx]));
	}

	return KwsJson;
}

PSpResult TSpSearchPoint::CreateNewResult(const TStr& QueryId, const TStr& QueryStr) const {
	return TSpResult::New(QueryId, QueryStr);
}

PSpResult TSpSearchPoint::GetResult(const TStr& QueryId, const TStr& QueryStr, const TInt& NResults) {
	PSpResult PResult = GetCachedResult(QueryId);
	if (!PResult.Empty()) {
		return PResult;
	}

	try {
		PResult = CreateNewResult(QueryId, QueryStr);
		DataSource->ExecuteQuery(PResult, NResults);
	} catch (const PExcept& Except) {
		Notify->OnNotifyFmt(TNotifyType::ntErr, "Failed to get result %s!", Except->GetMsgStr().CStr());
	}
	return PResult;
}

TStr TSpSearchPoint::GenQueryId(const TStr& QueryStr, const TStr& ClusteringKey, const int& NResults) const {
	return TMd5::GetMd5SigStr(QueryStr + "____" + ClusteringKey + "____" + TInt::GetStr(NResults));
}

PSpResult TSpSearchPoint::ExecuteQuery(const TStr& QueryStr, const TStr& ClusteringKey, const TInt& NResults) {
	PSpResult PResult = NULL;
	try {
		TStr QueryId = GenQueryId(QueryStr, ClusteringKey, NResults);

		PResult = GetResult(QueryId, QueryStr, NResults);
		return GenClusters(PResult, ClusteringKey);
	} catch (const PExcept& Except) {
		Notify->OnNotifyFmt(TNotifyType::ntErr, "Exception while executing query: %s", Except->GetMsgStr().CStr());
		return PResult;
	}
}

///////////////////////////////////////////////
// Query Manager
PSpQuery TSpQueryManager::NewQuery(const PSpResult& PResult) {
// 	const TStr& QueryId = PResult->QueryId;

//	EAssert(!IsQuery(QueryId));

	PSpQuery PQuery = TSpQuery::New(PResult);
	QueryH.AddDat(PQuery->Id, PQuery);
	return PQuery;
}

void TSpQueryManager::DelOutdated() {
	if (QueryH.Empty())
		return;

	TSecTm CurrStamp = TSecTm::GetCurTm();

	// iterate through all the Querys and remove the ones
	// with (Timestamp - Now) >= MaxIdleTime
	TVec<TStr> KeyV;	QueryH.GetKeyV(KeyV);
	for (int i = 0; i < KeyV.Len(); i++) {
		const TStr& Key = KeyV[i];
		PSpQuery PQuery = QueryH.GetDat(Key);

		const TSecTm& LastStamp = PQuery->TimeStamp;
		if (CurrStamp - LastStamp > TSpQueryManager::MaxIdleSecs)
			QueryH.DelKey(Key);
	}
}

PSpQuery TSpQueryManager::GetQuery(const TStr& QueryId) {
	EAssert(QueryH.IsKey(QueryId));
	PSpQuery& PQuery = QueryH.GetDat(QueryId);
	PQuery->Timestamp();

	DelOutdated();

	return PQuery;
}


/////////////////////////////////////////////////
//// Search Point - Demo Server
//TSpDemoSrv::TSpDemoSrv(const TStr& BaseUrl, const TStr& Path, const PSpSearchPoint& _PSearchPoint):
//		TSpAbstractServer(BaseUrl, Path, _PSearchPoint) {}
//
//PSIn TSpDemoSrv::ProcHtmlPgRq(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr) {
//	TStr Path = RqEnv->GetHttpRq()->GetUrl()->GetPathStr();
//
//	// throw an axception if anyone tries to access the mobile page directly
//	EAssert(Path.SearchStr("result_m.html") < 0);
//
//	bool IsResultReq = Path.IsStrIn("result.html");
//
//	// check for mobile user-agents
//	PHttpRq HttpRq = RqEnv->GetHttpRq();
//
//	PSIn SIn;
//	if (HttpRq->GetUsrAgentDeviceType() == uadtMobile && IsResultReq) {
//		// mobile client
//
//		// copied from TSASFunFPath::ExecSIn
//		// construct file name
//		TStr FNm = FPath;
//		PUrl Url = RqEnv->GetHttpRq()->GetUrl();
//		const int PathSegs = Url->GetPathSegs();
//		if (PathSegs  == 1) {
//			// nothing specified, do the default
//			TStr PathSeg = Url->GetPathSeg(0);
//			if (PathSeg.LastCh() != '/') { FNm += "/"; }
//			FNm += "index.html";
//		} else {
//			// extract file name
//			for (int PathSegN = 1; PathSegN < PathSegs; PathSegN++) {
//				FNm += "/"; FNm += Url->GetPathSeg(PathSegN);
//			}
//		}
//
//		FNm.ChangeStrAll("result.html", "result_m.html");
//		ContTypeStr = THttp::TextHtmlFldVal;
//
//		SIn = TFIn::New(FNm);
//	} else {
//		// a regular request from the client, call parent's function
//		SIn = TSASFunFPath::ExecSIn(FldNmValPrV, RqEnv, ContTypeStr);
//	}
//
//	// reading the parameters
//	if (IsResultReq) {
//		// collect the parameters
//		TStr QueryStr = GetFldVal(FldNmValPrV, TSpAbstractServer::QueryAttrName, "");
//		TStr ClusteringKey = GetFldVal(FldNmValPrV, TSpAbstractServer::ClusteringUtilsAttrName, "");
//		int NResults = TMath::Mx(GetFldInt(FldNmValPrV, TSpAbstractServer::NResultsAttrName, TSpAbstractServer::NResultsDefault), TSpAbstractServer::NResultsMin);
//
//		// substituting
//		// if the query string has been found => change the template in the file
//		// with the query string
//		{
//			TStr QueryId = GenQueryID(QueryStr, ClusteringKey, NResults);
//			// read the whole input stream and put it into
//			// another stream
//			TChA StrBuff = "";
//			TStr LnStr;
//			while (SIn->GetNextLn(LnStr)) {
//				LnStr.ChangeStrAll(TSpDemoSrv::ClusteringTargetStr, ClusteringKey);
//				LnStr.ChangeStrAll(TSpDemoSrv::QueryIDTargetStr, QueryId);
//				LnStr.ChangeStrAll(TSpDemoSrv::NResultsTargetStr, TInt::GetStr(NResults));
//
//				if (!QueryStr.Empty()) {
//					LnStr.ChangeStrAll(TSpDemoSrv::TitleTargetStr, QueryStr + " - SearchPoint");
//					LnStr.ChangeStrAll(TSpDemoSrv::QueryTargetStr, QueryStr);
//				} else {
//					LnStr.ChangeStrAll(TitleTargetStr, "SearchPoint");
//					LnStr.ChangeStrAll(QueryTargetStr, "");
//				}
//
//				StrBuff += LnStr + "\n";
//			}
//
//			return TMIn::New(StrBuff);
//		}
//	}
//	return SIn;
//}
//
//
/////////////////////////////////////////////////
//// Search Point - Abstract Server
//PSpResult TSpAbstractServer::ExecuteQuery(const TStr& QueryStr,
//		const TStr& ClusteringKey, const int NResults) {
//	return PSearchPoint->ExecuteQuery(QueryStr, ClusteringKey, NResults);
////	TStr QueryId = GenQueryID(QueryStr, ClusteringKey, NResults);
////
////	PSpResult PResult = PSearchPoint->GetResult(QueryId, QueryStr, NResults);
////	return PSearchPoint->GenClusters(PResult, ClusteringKey);
//}
//
//TStr TSpAbstractServer::GenQueryID(const TStr& QueryStr, const TStr& ClusteringKey, const int& NResults) const {
//	return PSearchPoint->GenQueryId(QueryStr, ClusteringKey, NResults);
////	return TMd5::GetMd5SigStr(QueryStr + "____" + ClusteringKey + "____" + TInt::GetStr(NResults));
//}
//
//PSIn TSpAbstractServer::ProcessPosUpdate(const TFltPrV& PosV, const int& Page, const TStr& QueryID) const {
//	PJsonVal ResultJson = PSearchPoint->ProcPosPageRq(PosV, Page, QueryID);
//	return TMIn::New(TJsonVal::GetStrFromVal(ResultJson));
//}
//
//PSIn TSpAbstractServer::ProcessKwsUpdate(const TFltPr& Pos, const TStr& QueryId) const {
//	PJsonVal ResultJson = PSearchPoint->ProcessPosKwRq(Pos, QueryId);
//	return TMIn::New(TJsonVal::GetStrFromVal(ResultJson));
//}
//
//PSIn TSpAbstractServer::ProcessQuery(const TStr& QueryStr, const TStr& ClusteringKey, const int& NResults) {
//
//	PSpResult PResult = ExecuteQuery(QueryStr, ClusteringKey, NResults);
//	TStr InsertJSon = !PResult.Empty() ? TJsonVal::GetStrFromVal(PSearchPoint->GenJSon(PResult, 0, PSearchPoint->PerPage)) : "";
//
//	return TMIn::New(InsertJSon);
//}
//
//PSIn TSpAbstractServer::ExecSIn(const TStrKdV& FldNmValPrV, const PSAppSrvRqEnv& RqEnv, TStr& ContTypeStr) {
//	TStr Path = RqEnv->GetHttpRq()->GetUrl()->GetPathStr();
//
//	if (Path.SearchStr("pos_page.html") >= 0) {
//		// process change position request
//		// get the parameters
//		EAssert(IsFldNm(FldNmValPrV, "x") && IsFldNm(FldNmValPrV, "y") && IsFldNm(FldNmValPrV, TSpAbstractServer::QueryIDAttrName));
//
//		// get x, y and page parameters
//		TFltPrV PosV;
//		TStr XStr = GetFldVal(FldNmValPrV, "x", "[" + TFlt::GetStr(TSpSearchPoint::MaxClusterX/2) + "]");
//		TStr YStr = GetFldVal(FldNmValPrV, "y", "[" + TFlt::GetStr(TSpSearchPoint::MaxClusterY/2) + "]");
//
//		// x and y are in format [x1,x2,...], [y1,y2,...] so parse them
//		XStr.DelChAll('[');	XStr.DelChAll(']');
//		YStr.DelChAll('[');	YStr.DelChAll(']');
//		TStrV XStrV;	XStr.SplitOnAllAnyCh(",", XStrV);
//		TStrV YStrV;	YStr.SplitOnAllAnyCh(",", YStrV);
//		for (int i = 0; i < XStrV.Len(); i++) {
//			const TStr XStr = XStrV[i];	const TStr YStr = YStrV[i];
//			const TFlt XFlt = (TStr("Infinity") == XStr) ? TFlt::PInf : XStr.GetFlt();
//			const TFlt YFlt = (TStr("Infinity") == YStr) ? TFlt::PInf : YStr.GetFlt();
//			PosV.Add(TFltPr(XFlt, YFlt));
//		}
//
//		TStr PageStr = GetFldVal(FldNmValPrV, "p", "0");
//		TStr QueryId = GetFldVal(FldNmValPrV, TSpAbstractServer::QueryIDAttrName, "");
//
//		int Page = PageStr.GetInt();
//
//		return ProcessPosUpdate(PosV, Page, QueryId);
//	} else if (Path.IsStrIn("query.html")) {
//		// fetch results and compute clusters
//		EAssert(IsFldNm(FldNmValPrV, TSpAbstractServer::QueryAttrName));
//
//		TStr QueryStr = GetFldVal(FldNmValPrV, TSpAbstractServer::QueryAttrName, "");
//
//		// get the engine, clustering and number of results to return
//		TStr ClusteringKey = GetFldVal(FldNmValPrV, TSpAbstractServer::ClusteringUtilsAttrName, "kmeans");
//		int NResults = TMath::Mx(GetFldInt(FldNmValPrV, TSpAbstractServer::NResultsAttrName, TSpAbstractServer::NResultsDefault), TSpAbstractServer::NResultsDefault);
//
//		return ProcessQuery(QueryStr, ClusteringKey, NResults);
//	} else if (Path.IsStrIn("keywords.html")) {
//		// process change position request
//		// get the parameters
//		EAssert(IsFldNm(FldNmValPrV, "x") && IsFldNm(FldNmValPrV, "y") && IsFldNm(FldNmValPrV, TSpAbstractServer::QueryIDAttrName));
//
//		// get x, y and page parameters
//		TFltPr Pos;
//		TStr XStr = GetFldVal(FldNmValPrV, "x", TFlt::GetStr(TSpSearchPoint::MaxClusterX/2));
//		TStr YStr = GetFldVal(FldNmValPrV, "y", TFlt::GetStr(TSpSearchPoint::MaxClusterY/2));
//
//		// x and y are in format [x1,x2,...], [y1,y2,...] so parse them
//		//XStr.DelChAll('[');	XStr.DelChAll(']');
//		//YStr.DelChAll('[');	YStr.DelChAll(']');
//
//		Pos.Val1 = (TStr("Infinity") == XStr) ? TFlt::PInf : XStr.GetFlt();
//		Pos.Val2 = (TStr("Infinity") == YStr) ? TFlt::PInf : YStr.GetFlt();
//
//		TStr QueryID = GetFldVal(FldNmValPrV, TSpAbstractServer::QueryIDAttrName, "");
//
//		return ProcessKwsUpdate(Pos, QueryID);
//	} else
//		// a normal page request
//		return ProcHtmlPgRq(FldNmValPrV, RqEnv, ContTypeStr);
//}