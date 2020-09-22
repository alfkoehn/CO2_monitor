const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <!-- make webpage fit to your browser, not matter what OS or browser -->
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- load Font Awesome, get integrity and url here: https://fontawesome.com/account/cdn --> 
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.14.0/css/all.css" integrity="sha384-HzLeBuhoNPvSl5KYnjx0BT+WB0QEEqLprO+NBkkk5gbc67FTaL7XIGa2w1L0Xbgc" crossorigin="anonymous">
  <!-- add some CSS style: font, size for header (h2) and paragraph (p) -->
  <!--                     size and format for labels to read           -->
  <style>
    html {
     font-family: Arial;
     display:     inline-block;
     margin:      0px auto;
     text-align:  center;
    }
    h2 { 
      font-size:  3.0rem; 
    }
    p { 
      font-size:  3.0rem; 
    }
    .units { 
      font-size:  1.2rem; 
    }
    .scd30-labels{
      font-size:      1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }
  </style>
</head>
<body>
  <h2>CO2 monitor</h2>
  <!-- paragraph for CO2 concentration -->
  <p>
    <i class="fas fa-head-side-cough" style="color:#ff6600;"></i> 
    <span class="scd30-labels">CO2 concentration</span> 
    <span id="co2">%CO2%</span>
    <sup class="units">ppm</sup>
  </p>
  <!-- paragraph for temperature -->
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="scd30-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&#8451</sup>
  </p>
  <!-- paragraph for humidity -->
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="scd30-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">%</sup>
  </p>
  <!-- paragraph for getting additional information (github) and getting into contact via twitter -->
  <p>
    <i class="fab fa-github" style="font-size:1.0rem;color:black;"></i>
    <span style="font-size:1.0rem;">The CO2 monitor on </span>
    <a href="https://github.com/alfkoehn/CO2_monitor" target="_blank" style="font-size:1.0rem;">GitHub (documentation + code)</a>
  </P>
  <p>
    <i class="fab fa-twitter" style="font-size:1.0rem;color:#1DA1F2;"></i>
    <span style="font-size:1.0rem;">Twitter: </span>
    <a href="https://twitter.com/formbar" target="_blank" style="font-size:1.0rem;">&#64;formbar</a>
  </P>
</body>
<!-- JavaScript to update CO2, temperature and humidity automatically -->
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("co2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/co2", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;
</script>
</html>)rawliteral";
