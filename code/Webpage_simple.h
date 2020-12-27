//
// PROGMEM stores data in flash memory, default is storing in SRAM
//   --> usually, only worth to be used for large block of data
// R infront of string indicates a RAW string literal
//   --> no need to escape linebreaks, quotationmarks etc.
//   --> allows to put full html-website into variable
//   --> beginning and end of RAW string literal indicated by
//       =====( ... )=====
//
const char MAIN_page[] PROGMEM = R"=====(
<!doctype html>
<html>

<head>
  <title>CO2 Monitor</title>
  <!-- very helpful reference: https://www.w3schools.com -->
  <meta charset="utf-8">
  <!-- make webpage fit to your browser, no matter what OS or browser (also adjusts font sizes) -->
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- prevent request on favicon, otherwise ESP receives favicon request every time web server is accessed -->
  <link rel="icon" href="data:,">
  <!-- load Font Awesome, get integrity and url here: https://fontawesome.com/account/cdn --> 
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.14.0/css/all.css" integrity="sha384-HzLeBuhoNPvSl5KYnjx0BT+WB0QEEqLprO+NBkkk5gbc67FTaL7XIGa2w1L0Xbgc" crossorigin="anonymous">
  <!-- unit 'rem' used in font sizes: values are relative to root html element (not parent element) -->
  <style>
    canvas{
      -moz-user-select: none;
      -webkit-user-select: none;
      -ms-user-select: none;
    }
    html {
     font-family      : Arial;
     display          : inline-block;
     margin           : 0px auto;
     text-align       : center;
    }
    .units {
      font-size       : 1.0rem;
    }
    .scd30-labels {
      font-size       : 1.0rem;
      padding-bottom  : 15px;
    }
  </style>
</head>

<body>
  <div style="text-align:center;">
    <p style="font-size:20px">
      <b>CO<sub>2</sub> Monitor</b>   
    </p>
    <p>
      <i class="fab fa-github" style="font-size:1.0rem;color:black;"></i>
      <a href="https://github.com/alfkoehn/CO2_monitor" target="_blank" style="font-size:1.0rem;">Documentation &amp; code on GitHub</a>
    </p>
    <p>
      <i class="fab fa-twitter" style="font-size:1.0rem;color:#1DA1F2;"></i>
      <span style="font-size:1.0rem;">Contact via twitter: </span>
      <a href="https://twitter.com/formbar" target="_blank" style="font-size:1.0rem;">&#64;formbar</a>
    </p>
  </div>
  <br>
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
<br>
<br>  

<script>

  // update intervall for getting new data in milliseconds
  var updateIntervall = 5000;

  setInterval(function() {
    // call a function repetitively, intervall set by variable <updateIntervall>
    getData();
  }, updateIntervall);
 
  function getData() {
    var xhttp           = new XMLHttpRequest();
    
    // onreadystatechange property defines a function to be executed when the readyState changes
    xhttp.onreadystatechange = function() {

      // "readyState" property holds the status of the "XMLHttpRequest"
      //          values: 0   --> request not initialized 
      //                  1   --> server connection established
      //                  2   --> request received
      //                  3   --> processing request 
      //                  4   --> request finished and response is ready
      // "status" values: 200 --> "OK"
      //                  403 --> "Forbidden"
      //                  404 --> "Page not found"
      // "this" keyword always refers to objects it belongs to
      if (this.readyState == 4 && this.status == 200) {
        //Push the data in array
  
        // Date() creates a Date object
        // toLocaleTimeString() returns time portion of Date object as string
        var time = new Date().toLocaleTimeString();

        // read-only XMLHttpRequest property responseText returns 
        // text received from a server following a request being sent
        var txt = this.responseText;

        // data from webserver is always a string, parsing with JSON.parse()
        // to let data beome a JavaScrip object
        var obj = JSON.parse(txt);
  
        // add elements to arrays
        // push() methods adds new items to end of array, returns new length
        document.getElementById("co2").innerHTML = obj.COO;
        document.getElementById("temperature").innerHTML = obj.Temperature;
        document.getElementById("humidity").innerHTML = obj.Humidity;

      }
    };
    xhttp.open("GET", "readData", true); //Handle readData server on ESP8266
    xhttp.send();
  }

</script>
</body>
</html>
)=====";
