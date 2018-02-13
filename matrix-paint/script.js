/* global $ */
var matrixSize = 10;

var showSaved = function() {
  // Try localstorage
  var pictures = JSON.parse(window.localStorage.getItem("pictures")) || {};
  var $saved = $("#saved");
  $saved.find("div").remove();
  Object.keys(pictures).forEach(function(key) {
    var picture = pictures[key];
    $saved.append("<div class='pic'><h7>" + key + "</h7><input type=text value='" + picture + "'><button class='load'>Load</button><button class='delete' data-name='" + key + "'>Delete</button><div>");
  });
};

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
  window.code = code;
});

$("#save").on("click", function() {
  var name = $("[name=name]").val();
  var pictures = JSON.parse(window.localStorage.getItem("pictures")) || {};
  pictures[name] = window.code;
  window.localStorage.setItem("pictures", JSON.stringify(pictures));
  showSaved();
});

var drawCode = function(code) {
  $("[data-i]").removeClass("active");
  code.forEach(function(ix) {
    $("[data-i='" + ix + "']").addClass("active");
  });
};

$("#export").on("click", function() {
  var codes$ = $(".pic input[type=text]");
  var codes = [];
  var p;

  // Max
  var max = 0;
  codes$.each(function() {
    var l = this.value.split(",").length;
    if (max < l) {
      max = l;
    }
  });

  codes$.each(function() {
    p = $(this).prev();
    var v = this.value.split(",");
    var vl = v.length;

    for (var i2 = 0; i2 < max - vl; i2++) {
      v.push("-1");
    }

    codes.push("{" + v.join(",") + "}, // " + p.text());
  });
  $("textarea").val(codes.join("\n"));
});

$(document).on("click", ".load", function() {
  var code = $(this).prev().val().split(",");
  drawCode(code);
  $("textarea").val("const int leds[" + code.length + "] = {" + code.join(",") + "};");
  window.code = code;
});

$(document).on("click", ".delete", function() {
  var pictures = JSON.parse(window.localStorage.getItem("pictures"));
  delete pictures[this.dataset.name];
  window.localStorage.setItem("pictures", JSON.stringify(pictures));
  showSaved();
});

showSaved();
