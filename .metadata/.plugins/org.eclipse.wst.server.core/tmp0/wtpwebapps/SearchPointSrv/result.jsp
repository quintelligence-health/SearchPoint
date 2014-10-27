<%@page import="si.ijs.searchpoint.SearchPoint"%>
<%@page import="si.ijs.searchpointsrv.config.Configuration"%>
<%@ page language="java" contentType="text/html; charset=UTF-8"
    pageEncoding="UTF-8"%>
<!DOCTYPE HTML>
<html>
<%
	String query = request.getParameter("q");
	String clustering = request.getParameter("c");
	int limit;
	try {
		limit = request.getParameterMap().containsKey("n") ? Integer.parseInt(request.getParameter("n")) : Configuration.DEFAULT_LIMIT;
	} catch (NumberFormatException ex) {
		limit = Configuration.DEFAULT_LIMIT;
	}
%>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
	<title><% out.print(query); %> - SearchPoint</title>
	
	<link type="text/css" rel="Stylesheet" href="css/searchpoint.css" />
	<link type="text/css" rel="Stylesheet" href="css/tooltip.css" />
	<link type="text/css" rel="Stylesheet" href="css/jquery-ui.min.css" />
	
	<script type="application/javascript" src="js/kinetic-v3.8.2.js"></script>
	<script type="application/javascript" src="js/jquery-1.10.2.js"></script>
	<script type="application/javascript" src="js/jquery-ui-1.10.4.custom.min.js"></script>
	<script type="application/javascript" src="js/sp.js"></script>
	<script type="application/javascript" src="js/tagcloud.js"></script>
	<script>
	  (function(i,s,o,g,r,a,m){i['GoogleAnalyticsObject']=r;i[r]=i[r]||function(){
	  (i[r].q=i[r].q||[]).push(arguments)},i[r].l=1*new Date();a=s.createElement(o),
	  m=s.getElementsByTagName(o)[0];a.async=1;a.src=g;m.parentNode.insertBefore(a,m)
	  })(window,document,'script','//www.google-analytics.com/analytics.js','ga');
	
	  ga('create', 'UA-56055149-1', 'auto');
	  ga('send', 'pageview');
	</script>
	<script type="application/javascript">
		var stageW = 500;
		var stageH = 500;
	</script>
</head>
<body>
	<header>
		<table id="tbl-search">
			<tbody>
				<tr>
					<td>
						<a href="index.html" title="Go to SearchPoint Home">
							<img src="images/logo.jpg" width="128" height="52" style="border: none" alt="(no image)" />
						</a>
					</td>
					<td>
						<input id="q" name="q" type="text" value="<% out.print(query); %>" maxlength="2048" size="41" />
					</td>
					<td>
						<input name="search" id="kmeans" value="Search with topics" type="button" onclick="service.fetchData(this.id, null);" />
						<input name="search" id="dmoz" value="Search with dmoz.org" type="button" onclick="service.fetchData(this.id, null);" />
						<input type="hidden" id="clustering_fld" name="c" value="<% out.print(clustering); %>" />
						<input type="hidden" id="s" name="s" value="<% out.print(SearchPoint.getInstance().getQueryId(query, clustering, limit)); %>" disabled="disabled" />
					</td>
				</tr>
			</tbody>
		</table>
		<div id="advanced" style="display: none;">
			<span class="form-entry">
				<label for="spn-nresults">Number of search results:</label>
				<input id="spn-nresults" type="text" min="200" step="50" value="<% out.print(limit); %>" />
			</span>
		</div>
		<div id="advanced-toggle"></div>
		<script type="application/javascript">
			$('#q').keypress(function (event) {
				if (event.which == 13 || event.keyCode == 13)
					service.fetchData($('#clustering_fld').val());
			});
			$('#spn-nresults').spinner({
				min: 200,
				step: 50
			});
			$('#spn-nresults').keypress(function(event) {
				event.preventDefault();
				return false;
			});
			$('#spn-nresults').keydown(function(event) {
				if (event.which == 13 || event.keyCode == 13)
					service.fetchData($('#clustering_fld').val());
				else {
					event.preventDefault();
					return false;
				}
			});
			$('#advanced-toggle').click(function() {
				if ($('#advanced').css('display') == 'none') {
					$('#advanced').slideDown(500, function() {
						$('#advanced-toggle').css('background-image', 'url("images/up.png")');
					});
				} else {
					$('#advanced').slideUp(500, function() {
						$('#advanced-toggle').css('background-image', 'url("images/down.png")');
					});
				}
			});
		</script>
	</header>
	<section id="data_section">
		<section id="data"></section>
		<footer>
			<br />
			<nav id="nav"></nav>
			<div id="about">
				<br />
				<br />
				<p class="about">
					<a href="http://searchpoint.ijs.si/About.htm">About</a> - ©2007-2011 SearchPoint
				</p>
			</div>
		</footer>
	</section>
	<aside>
		<section id="body_cluster_canvas" onselectstart="return false;"></section>
	</aside>
</body>
</html>
