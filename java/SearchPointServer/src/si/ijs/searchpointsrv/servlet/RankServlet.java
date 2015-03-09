package si.ijs.searchpointsrv.servlet;

import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.servlet.ServletException;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;

import si.ijs.searchpoint.SearchPoint;
import si.ijs.searchpointsrv.config.Configuration;

/**
 * Servlet implementation class RankServlet
 */
@WebServlet("/rank")
public class RankServlet extends HttpServlet {
	
	private static final long serialVersionUID = -3150319959431107958L;
	
	private static final Logger log = Logger.getLogger(RankServlet.class.getName());

	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		try {
			String queryId = request.getParameter(Configuration.QUERY_ID_PARAM);
			int page = request.getParameterMap().containsKey(Configuration.PAGE_PARAM) ? Integer.parseInt(request.getParameter(Configuration.PAGE_PARAM)) : 0;
			String posStr = request.getParameter(Configuration.POSITIONS_PARAM);

			JSONArray posVJson = (JSONArray) JSONValue.parse(posStr);
			JSONObject posJson = (JSONObject) posVJson.get(0);
			
			double x = Configuration.castPos(posJson.get("x"));
			double y = Configuration.castPos(posJson.get("y"));
			
			if (log.isLoggable(Level.FINE))
				log.info(String.format("Received rank request queryId: %s, x: %.2f, y: %.2f, page: %d", queryId, x, y, page));
			
			String result = SearchPoint.getInstance().rankByPos(x, y, page, queryId);
			
			response.getWriter().write(result);
		} catch (Throwable t) {
			log.log(Level.SEVERE, "Failed to process rank request!", t);
			response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR);
		}
	}
}