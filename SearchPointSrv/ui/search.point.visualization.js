let {initiateQuery, service} = require("./sp");


$(document).ready(function () {
  var query = decodeURI($(document).getUrlParam('q'));
  var clust = $(document).getUrlParam('c');
  var limit = $(document).getUrlParam('n');

  if (limit == null || limit == '') limit = 200;

  $('#q').val(query);
  $('#clustering_fld').val(clust);
  $('#spn-nresults').val(limit);

  $('#q').keypress(function (event) {
    if (event.which == 13 || event.keyCode == 13)
      service.fetchData();
  });
  $('#spn-nresults').spinner({
    min: 200,
    step: 50
  }).val(limit);
  $('#spn-nresults').keypress(function (event) {
    event.preventDefault();
    return false;
  });
  $('#spn-nresults').keydown(function (event) {
    if (event.which == 13 || event.keyCode == 13)
      service.fetchData();
    else {
      event.preventDefault();
      return false;
    }
  });
  $('#advanced-toggle').click(function () {
    if ($('#advanced').css('display') == 'none') {
      $('#advanced').slideDown(500, function () {
        $('#advanced-toggle').css('background-image', 'url("SearchPoint/images/up.png")');
      });
    } else {
      $('#advanced').slideUp(500, function () {
        $('#advanced-toggle').css('background-image', 'url("SearchPoint/images/down.png")');
      });
    }
  });
  initiateQuery();

});
$('#search-btn').on('click', function () {
  service.fetchData();
});

$('#source').on('change', function () {
  console.log("source changed");
  let val = $('#source option:selected').val();
  let display = val === "MAG" ? "table-cell" : "none"
  $('#publication-type-field').css('display', display);
  $('#start-year-field').css('display', display);
  $('#end-year-field').css('display', display);
})

$('.option').on('change', function () {
  service.fetchData();
})
