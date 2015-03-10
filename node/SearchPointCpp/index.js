var sp = require('bindings')('sp.node');
var httpsync = require('http-sync');
var math = require('mathjs');

// BING data source
{
	var apiKeys = [
		"IWBf7jPfsFw7m2QrNM493NWYTYiJ0ynYXVejNWA6kkc",
		"Y8O7wAHCl7z/DIlISSbZbAPDGT7waINUCSMC89gAHGA=",
		"agzOfae9TB8CsLkJ8rxSN/fq5QjG4G1H+RW+JzYMnbY"
	]
	
	var MAX_PER_QUERY = 50;
	
	for (var i = 0; i < apiKeys.length; i++) {
		var key = apiKeys[i];
		var encoded = new Buffer(':' + key).toString('base64');	
		apiKeys[i] = encoded;
	}
	
	function parseBingResp(data) {		
		try {
			var dataJson = JSON.parse(data);
			var result = [];
			
			dataJson.d.results.forEach(function (val) {
				result.push({
					title: val.Title,
					description: val.Description,
					url: val.Url,
					displayUrl: val.DisplayUrl
				})
			});
			
			return { items: result, hasNext: '__next' in dataJson.d };
		} catch (e) {
			console.log(e.message);
		}
		
		return null;
	}
	
	sp.fetchBing = function (query, limit) {		
		var result = [];
		var nQueries = Math.ceil(limit / MAX_PER_QUERY);
		
		var hasNext = true;
		var offset = 0;
		var i = 0;
		while (hasNext && i++ < nQueries) {
			try {
				// select the API key
				var apiKey = apiKeys[math.randomInt(apiKeys.length)];
				var top = Math.min(limit - offset, MAX_PER_QUERY);
				
				var opts = {
					host: 'api.datamarket.azure.com',
					path: '/Data.ashx/Bing/SearchWeb/v1/Web?Query=%27' + query + '%27&$skip=' + offset + '&$top=' + top + '&$format=JSON',
					protocol: 'https',
					method: 'GET',
					headers: {
						'Authorization': 'Basic ' + apiKey
					}
				}
				
				var req = httpsync.request(opts);
				var resp = req.end();
				
				var parsed = parseBingResp(resp.body.toString());
				
				if (parsed == null) break;
				
				result = result.concat(parsed.items);
				hasNext = parsed.hasNext;
				
				offset += parsed.items.length;
			} catch (e) {
				console.log(e.message);
				break;
			}
		}
				
		return result;
	}
}

module.exports = exports = sp;