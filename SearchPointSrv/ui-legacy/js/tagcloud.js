function createWc(words, centerX, centerY, scale) {
	var n = words.length;
	var placed = [];
	
	scale = scale == null ? 1 : scale;

	// map weights to range [0, 1]
	var maxWgt = -1;
	for (var i = 0; i < words.length; i++) {
		if (words[i].weight > maxWgt) 
			maxWgt = words[i].weight;
	}
	
	for (var i = 0; i < words.length; i++) {
		words[i].weight /= maxWgt;
	}
	
	// calc word sizes and max area
	var maxArea = -1;
	for (var i = 0; i < n; i++) {
		var word = words[i];
		word.height = Math.ceil(getWordH(word)*scale);
		word.width = getWordW(word);
		
		if (word.width * word.height > maxArea)
			maxArea = word.width * word.height;
	}
	
	// calc new weights by area
	for (var i = 0; i < n; i++) {
		words[i].newWgt = (words[i].width * words[i].height) / maxArea;
	}
	
	// sort the words by weight
	words.sort(function (w0, w1) {
		return w1.newWgt - w0.newWgt;
	});
	
	// required functions
	function intersects(word0, word1) {
		return Math.abs(word0.pos[1] - word1.pos[1]) <= (word0.height + word1.height) / 2 &&
				Math.abs(word0.pos[0] - word1.pos[0]) <= (word0.width + word1.width) / 2;
	}
	
	function intersectsPlaced(word) {
		for (var i = 0; i < placed.length; i++) {
			if (intersects(word, placed[i]))
				return true;
		}
		return false;
	}
	
	// place the words
	var rstep = 3.0;
	var astep = .1;
	for (var i = 0; i < n; i++) {
		var word = words[i];
		
		var angle = 2*Math.PI * Math.random();
		var radius = 0;
		var dir = (i % 2)*2 - 1;
		
		word.pos = [centerX, centerY];
		var isPlaced = false;
		while (!isPlaced) {
			for (var addAngle = 0; addAngle < 2*Math.PI; addAngle += astep) {
				var alpha = dir*(angle + addAngle);
				word.pos[0] = centerX + radius*Math.cos(alpha);
				word.pos[1] = centerY + radius*Math.sin(alpha);
        
				if (!intersectsPlaced(word)) {
					isPlaced = true;
					break;
				}
			}
			radius += rstep;
		}
		placed.push(word);
	}
}

function getWordH(word) {
	return Math.max(minKwSize, word.fq * maxKwSize);
}

function getWordW(word) {
	var ctx = stages[0].layers[1].getContext();
	ctx.font = word.height + 'pt Calibri';
	var metrics = ctx.measureText(word.text);
    return metrics.width;
}
