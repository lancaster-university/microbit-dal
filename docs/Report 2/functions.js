function init() {
	var d = new Date();
  var n = d.toLocaleDateString();
	document.getElementById("id_date").innerHTML = n;
}
