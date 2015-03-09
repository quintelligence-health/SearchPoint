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
 * Servlet implementation class KeywordsServlet
 */
@WebServlet("/keywords")
public class KeywordsServlet extends HttpServlet {

	private static final long serialVersionUID = 7141635444337783583L;
	
	private static final Logger log = Logger.getLogger(KeywordsServlet.class.getName());

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		try {
			String queryId = request.getParameter(Configuration.QUERY_ID_PARAM);
			double x = Double.parseDouble(request.getParameter("x"));
			double y = Double.parseDouble(request.getParameter("y"));
			
			if (log.isLoggable(Level.FINE))
				log.fine(String.format("Received keyword request queryId: %s, x: %.2f, y: %.2f", queryId, x, y));
			
			String result = SearchPoint.getInstance().fetchKeywords(x, y, queryId);
			
			response.getWriter().write(result);
		} catch (Throwable t) {
			log.log(Level.SEVERE, "Failed to process rank request!", t);
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
		}
	}
}