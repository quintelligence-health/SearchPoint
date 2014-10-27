package si.ijs.searchpoint;

import java.io.IOException;
import java.util.Set;

public class SearchPoint {

	private static SearchPoint instance;
	
	static {
		try {
			System.loadLibrary("SearchPointJava");
		} catch (UnsatisfiedLinkError e) {
			e.printStackTrace();
			System.exit(1);
		}
	}
	
	public static synchronized SearchPoint getInstance() {
		if (instance == null) {
			instance = new SearchPoint();
		}
		return instance;
	};
	
	private SearchPoint() {}
	
	public void init(Set<String> bingApiKeys, String unicodeDefPath) throws IOException {
		init(bingApiKeys, unicodeDefPath, null);
	}
	
	public void init(Set<String> bingApiKeys, String unicodeDefPath, String dmozDatFile) throws IOException {
		init(bingApiKeys.toArray(new String[bingApiKeys.size()]), unicodeDefPath, dmozDatFile);
	}
		
	private native void init(String[] bingApiKeys, String unicodeDefPath, String dmozDatFile) throws IOException;
	
	public native String processQuery(String query, String clustKey, int limit);
	
	public native String rankByPos(double x, double y, int page, String queryId);
	
	public native String fetchKeywords(double x, double y, String queryId);
	
	public native String getQueryId(String query, String clustering, int limit);
	
	public native void close();
}
