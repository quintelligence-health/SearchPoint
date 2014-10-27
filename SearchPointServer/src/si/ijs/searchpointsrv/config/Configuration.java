package si.ijs.searchpointsrv.config;

import java.io.IOException;
import java.util.Base64;
import java.util.HashSet;
import java.util.Properties;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;

public class Configuration {
	
	private static final Logger log = Logger.getLogger(Configuration.class.getName());

	public static final int DEFAULT_LIMIT = 200;
	
	public static final String QUERY_PARAM = "q";
	public static final  String CLUSTERING_PARAM = "c";
	public static final String LIMIT_PARAM = "n";
	public static final String QUERY_ID_PARAM = "qid";
	public static final String PAGE_PARAM = "p";
	public static final String POSITIONS_PARAM = "pos";
	
	public static String UNICODE_DEF_PATH;
	public static String DMOZ_DEF_PATH;
	
	public static Set<String> BING_API_KEYS;
	
	static {
		try {
			Properties props = new Properties();
			props.load(Configuration.class.getClassLoader().getResourceAsStream("searchpoint.properties"));
		
			UNICODE_DEF_PATH = props.getProperty("glib.unicode.file");
			DMOZ_DEF_PATH = props.containsKey("glib.dmoz.file") ? props.getProperty("glib.dmoz.file") : null;
			
			BING_API_KEYS = new HashSet<String>();
			String[] apiKeys = props.getProperty("bing.search.apikey").split(",");
			for (String apiKey : apiKeys) {
				String apiKey64 = Base64.getEncoder().encodeToString((":" + apiKey).getBytes());
				BING_API_KEYS.add(apiKey64);
			}
		} catch (IOException e) {
			log.log(Level.SEVERE, "Failed to load properties file!", e);
		}
	}
	
	/**
	 * Casts a number which is returned by google's json parser.
	 * 
	 * @param pos
	 * @return
	 */
	public static double castPos(Object pos) {
		if (pos instanceof Long || pos instanceof Integer)
			return (double) ((long) pos);
		else
			return (double) pos;
	}
}
