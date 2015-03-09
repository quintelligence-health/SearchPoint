#include "sp.h"

class TReiStr{
public:
  char* inner;
private:

public:
  TReiStr(): inner(new char[1]) { inner[0] = 0; }
  TReiStr(const char* CStr): inner(new char[strlen(CStr)+1]) {strcpy(inner, CStr);}
  ~TReiStr(){
	  delete[] inner;
  }
  TReiStr& operator=(const TReiStr& Str) {
	  delete[] inner;
	  inner=new char[strlen(Str.inner)+1];
	  strcpy(inner, Str.inner);
	  return *this;
  }
};

int main(int argc, char* argv[]){
#ifndef NDEBUG
    // report we are running with all Asserts turned on
    printf("*** Running in debug mode ***\n");
    setbuf(stdout, NULL);
#endif

	try {
//		// create environment
//		Env=TEnv(argc, argv, TNotify::StdNotify);
//		// command line parameters
//		Env.PrepArgs("Search Point Server", 0);
//		const int PortN = Env.GetIfArgPrefixInt("-port:", 8080, "Server Port");
//		if (Env.IsEndOfRun()){
//			return 0;
//		}
//
//		const TStr BaseUrl = "sp";
//		const TStr ClientPath = "../SearchPointClient";
//		const PNotify Notify = TStdNotify::New();
//
//		// load Unicode definition
//		TUnicodeDef::Load("./dbs/UnicodeDef.bin");
//
//		//create and register the servlets
//		TSAppSrvFunV SrvFunV;
//
//		//creating clustering utils
//		TStr DefaultClustUtils = "kmeans";
//		THash<TStr, PSpClustUtils> ClustUtilsH;
//		ClustUtilsH.AddDat(DefaultClustUtils, TSpDPMeansClustUtils::New());
//		//ClustUtilsH.AddDat(DefaultClustUtils, TSpDPMeansClustUtils::New(.06));
////		ClustUtilsH.AddDat("dmoz", TSpDmozClustUtils::New("dmoz_data/DMoz.dat"));
//
//		// old API key: Olk4Tzd3QUhDbDd6L0RJbElTU2JaYkFQREdUN3dhSU5VQ1NNQzg5Z0FIR0E9
//		// :Y8O7wAHCl7z/DIlISSbZbAPDGT7waINUCSMC89gAHGA=
//
//		// new API key: OlhCWFVlNDl2WEIxRnpzc0NmRWdPN0NoeWVpaWl1NDIrS3dZZTB1YnE4V2s9
//		// new API key: OklXQmY3alBmc0Z3N20yUXJOTTQ5M05XWVRZaUoweW5ZWFZlak5XQTZra2M9PQ==
//		// :XBXUe49vXB1FzssCfEgO7Chyeiiiu42+KwYe0ubq8Wk=
//		// :IWBf7jPfsFw7m2QrNM493NWYTYiJ0ynYXVejNWA6kkc=
//
//		TStrV ApiKeyV; ApiKeyV.Add("OklXQmY3alBmc0Z3N20yUXJOTTQ5M05XWVRZaUoweW5ZWFZlak5XQTZra2M=");
//
//		//creating a SearchPoint object
//		const PSpSearchPoint PSearchPoint = TSpSearchPointImpl::New(
//				ClustUtilsH,
//				DefaultClustUtils,
//				10,
//				TSpBingEngine::New(ApiKeyV, Notify),
//				Notify
//		);
//
//		SrvFunV.Add(TSpDemoSrv::New(BaseUrl, ClientPath, PSearchPoint));
//		SrvFunV.Add(TSASFunFile::New("favicon.ico", "../SearchPointClient/images/logo.ico", "image/ico"));
//		PWebSrv WebSrv = TSAppSrv::New(PortN, SrvFunV, Notify, true, false);
//
//		// wait for requests
//		while (true) {
//			try {
//				TLoop::Run();
//			} catch (...) {
//				Notify->OnNotify(TNotifyType::ntErr, "Exception on socket loop!");
//			}
//		}
//
//		return 0;

		TVec<TReiStr> ReiVec;
		TStrV Vec;

		const unsigned int nthreads = 6;
		unsigned int vsize = 1000003;
		unsigned int cycle = 1000000000;
		unsigned int t = nthreads; unsigned int i = 0;

		for (i = 0; i < vsize; i++) {
			Vec.Add(TInt(i).GetStr());
			ReiVec.Add((TInt(i).GetStr()).CStr());
			printf("%s\n", ReiVec[i].inner);
		}

		uint64 SinStart = TTm::GetCurUniMSecs();
		for (int i = 0; i < cycle; i++) {
			const TReiStr ReiStr = ReiVec[i % vsize];
			const TStr Str = Vec[i % vsize];

//			if (i % 10000 == 0)
//				printf("%d\n", i);
//			if (i > cycle / 2) {
//				printf("%d\n", i);
//			}
		}

		for (i = 0; i < vsize; i++) {
			printf("%s\n", ReiVec[i].inner);
		}

		uint64 SinDur = TTm::GetCurUniMSecs() - SinStart;
		printf("Single Rei Dur: %ld\n", SinDur);

		uint64 ParStart = TTm::GetCurUniMSecs();
		#pragma omp parallel for num_threads(nthreads)
		for (t = 0; t < nthreads; t++) {
			for (int i = 0; i < cycle; i++) {
				const TReiStr ReiStr = ReiVec[(nthreads*i + t) % vsize];
			}
		}
		uint64 ParDur = TTm::GetCurUniMSecs() - ParStart;
		printf("Par Rei Dur: %ld\n", ParDur);



//		int n = 100000000;
//		int nthreads = 6;
//		int vsize = nthreads*100000;
//
//		TStrV Vec;
//
//		for (int i = 0; i < vsize; i++) {
//			Vec.Add(TInt(i).GetStr());
//		}
//
//		uint64 Start = TTm::GetCurUniMSecs();
//
//		int t = 2;
//
//		for (int i = 0; i < n; i++) {
//			const TStr Str = Vec[(nthreads*i + t) % vsize];
//		}
//
//		uint64 Dur = TTm::GetCurUniMSecs() - Start;
//
//		printf("Single Dur: %ld\n", Dur);
//
//		Start = TTm::GetCurUniMSecs();
//
//		#pragma omp parallel for num_threads(6)
//		for (t = 0; t < 6; t++) {
//			for (int i = 0; i < n; i++) {
//				const TStr Str = Vec[(nthreads*i + t) % vsize];
//			}
//		}
//
//		Dur = TTm::GetCurUniMSecs() - Start;
//
//		printf("Multi Dur: %ld\n", Dur);
//
//		return 0;
	} catch (const PExcept& Except) {
		ErrNotify(Except->GetMsgStr());
	}
	
	return 1;
}
