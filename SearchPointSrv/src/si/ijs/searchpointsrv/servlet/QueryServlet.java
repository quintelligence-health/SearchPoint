package si.ijs.searchpointsrv.servlet;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import si.ijs.searchpoint.SearchPoint;
import si.ijs.searchpointsrv.config.Configuration;

/**
 * Servlet implementation class SearchPointServlet
 */
@WebServlet("/query")
public class QueryServlet extends HttpServlet {

	private static final long serialVersionUID = -2777902232370251627L;
	
	private static final Logger log = Logger.getLogger(QueryServlet.class.getName());

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		try {
			String query = request.getParameter(Configuration.QUERY_PARAM);
			String clustering = request.getParameter(Configuration.CLUSTERING_PARAM);
			int limit = request.getParameterMap().containsKey(Configuration.LIMIT_PARAM) ? Integer.parseInt(request.getParameter(Configuration.LIMIT_PARAM)) : Configuration.DEFAULT_LIMIT;
		
			if (log.isLoggable(Level.FINE))
				log.log(Level.FINE, String.format("Received query request query: %s, clustering: %s, limit: %d", query, clustering, limit));
			
			String result = SearchPoint.getInstance().processQuery(query, clustering, limit);
			
			response.getWriter().write(result);
		} catch (Throwable t) {
			log.log(Level.SEVERE, "Failed to process query request!", t);
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
		}
	}
}
