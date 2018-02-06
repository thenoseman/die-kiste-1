/* global $ */
var matrixSize = 10;

for (var o = 0; o < matrixSize; o++) {
  var html = [ "<tr>" ];

  for (var i = 0; i < matrixSize; i++) {
    var start = matrixSize - o - 1;
    var index = start + i * 10;
    html.push("<td data-i='" + index + "'>" + index + "</td>");
  }

  html.push("</tr>");
  $("table").append(html.join(""));
}
$("table").append("<tr><td>↑ DIN</td><td colspan='" + (matrixSize - 1) + "'></td></tr>");

$("#container td").on("click", function() {
  $(this).toggleClass("active");

  var code = [];
  for (var l = 0; l < matrixSize * matrixSize; l++) {
    var isActive = $("[data-i='" + l + "']").hasClass("active");
    if (isActive) {
      code.push(l);
    }
  }

  $("textarea").val("byte ledsToTurnOn[] = {" + code.join(",") + "};");
});
