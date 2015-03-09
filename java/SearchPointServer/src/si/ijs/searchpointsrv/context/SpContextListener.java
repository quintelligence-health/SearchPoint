package si.ijs.searchpointsrv.context;

import java.util.logging.Level;
import java.util.logging.Logger;

import javax.servlet.ServletContextEvent;
import javax.servlet.ServletContextListener;

import si.ijs.searchpoint.SearchPoint;
import si.ijs.searchpointsrv.config.Configuration;

public class SpContextListener implements ServletContextListener {
	
	private static final Logger log = Logger.getLogger(SpContextListener.class.getName());

	/*
	 * (non-Javadoc)
	 * @see javax.servlet.ServletContextListener#contextInitialized(javax.servlet.ServletContextEvent)
	 */
	@Override
	public void contextInitialized(ServletContextEvent arg0) {
		log.info("Initializing searchpoint ...");
		
		try {
			SearchPoint.getInstance().init(Configuration.BING_API_KEYS, Configuration.UNICODE_DEF_PATH, Configuration.DMOZ_DEF_PATH);
		} catch (Throwable t) {
			log.log(Level.SEVERE, "Failed to initialize search point!", t);
		}
		
		log.info("Searchpoint initialized!");
	}
	
	/*
	 * (non-Javadoc)
	 * @see javax.servlet.ServletContextListener#contextDestroyed(javax.servlet.ServletContextEvent)
	 */
	@Override
	public void contextDestroyed(ServletContextEvent arg0) {
		log.info("Destroying searchpoint ...");
		SearchPoint.getInstance().close();
		log.info("Searchpoint destroyed!");
	}
}