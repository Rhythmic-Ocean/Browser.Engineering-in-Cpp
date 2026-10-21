var strong = document.querySelectorAll("strong")[0];
var allow_submit = true;

function lengthCheck() {
  var value = this.getAttribute("value");
  allow_submit = value.length <= 100;
  if (!allow_submit) {
    strong.innerHTML = "Comment too long!";
  } else {
    strong.innerHTML = "";
  }
}

var form = document.querySelectorAll("form")[0]
form.addEventListener("submit", function (e) {
  e.preventDefault();
  x = new XMLHttpRequest();
  x.open("GET", "http://127.0.0.1:8000/", false);
  x.send();
})

var inputs = document.querySelectorAll("input");
for (var i = 0; i < inputs.length; ++i) {
  inputs[i].addEventListener("keydown", lengthCheck);
}
