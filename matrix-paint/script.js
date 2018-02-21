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

var displayCode = function() {
  var code = [];
  var bCode = [];
  var cByte = 0;

  for (var l = 0; l < matrixSize * matrixSize; l++) {
    var isActive = $("[data-i='" + l + "']").hasClass("active");

    if (isActive) {
      cByte = (cByte | Math.pow(2,(l % 8)));
      code.push(l);
    }

    if (l > 0 && (l + 1) % 8 === 0 || l === (matrixSize * matrixSize) - 1) {
      bCode.push(cByte);
      cByte = 0;
    }
  }

  var bytesFiltered = bCode.filter(function(n) {
    return n !== undefined;
  });

  var le = bytesFiltered.length;
  for (var i3 = le; i3 < 13; i3++) {
    bytesFiltered.push(0);
  }

  $("textarea").val("byte binary[12] = {" + bytesFiltered.join(",") + "};");
  window.code = code;
};

$("#container td").on("click", function() {
  $(this).toggleClass("active");
  displayCode();
});

var save = function(aname) {
  var name = aname || $("[name=name]").val();
  var pictures = JSON.parse(window.localStorage.getItem("pictures")) || {};
  pictures[name] = window.code;
  window.localStorage.setItem("pictures", JSON.stringify(pictures));
  showSaved();
};

$("#save").on("click", function() {
  save();
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
  displayCode();
});

$(document).on("click", ".delete", function() {
  var pictures = JSON.parse(window.localStorage.getItem("pictures"));
  delete pictures[this.dataset.name];
  window.localStorage.setItem("pictures", JSON.stringify(pictures));
  showSaved();
});

$("#import").on("click", function() {
  var matches = $("textarea").val().match(/\{([0-9 ,]+)\}/g);
  var code = [];
  matches.forEach(function(match, i2) {
    code.push("byte imported[] = " + match + ";");
    window.code = match.replace(/[{}]/,"").split(",");
    save("imported-" + i2);
  });
  $("textarea").val(code.join("\n"));
});

showSaved();
