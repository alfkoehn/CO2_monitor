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
  <!-- make webpage fit to your browser, not matter what OS or browser (also adjusts font sizes) -->
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!-- prevent request on favicon, otherwise ESP receives favicon request every time web server is accessed -->
  <link rel="icon" href="data:,">
  <!-- load Font Awesome, get integrity and url here: https://fontawesome.com/account/cdn --> 
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.14.0/css/all.css" integrity="sha384-HzLeBuhoNPvSl5KYnjx0BT+WB0QEEqLprO+NBkkk5gbc67FTaL7XIGa2w1L0Xbgc" crossorigin="anonymous">
  <!-- load chart.js library, this could also copied to esp8266 for usuage without internet connection -->
  <script src = "https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.3/Chart.min.js"></script>
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

    /* data table styling */
    #dataTable {
      font-family     : "Trebuchet MS", Arial, Helvetica, sans-serif;
      border-collapse : collapse;
      width           : 100%;
    }
    #dataTable td, #dataTable th {
      border          : 1px solid #ddd;
      padding         : 8px;
    }
    #dataTable tr:nth-child(even){
      background-color: #f2f2f2;
    }
    #dataTable tr:hover {
      background-color: #ddd;
    }
    #dataTable th {
      padding-top     : 12px;
      padding-bottom  : 12px;
      text-align      : left;
      background-color: #4CAF50;
      color           : white;
    }
  </style>
</head>

<body>
  <div style="text-align:center;">
    <p style="font-size:20px">
      <b>CO<sub>2</sub> Monitor: data logger (using Chart.js)</b>   
      <button type="button" onclick="downloadData()">Download data</button>
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
  <div class="chart-container" position: relative; height:350px; width:100%">
      <canvas id="Chart1" width="400" height="400"></canvas>
  </div>
  <br>
  <div class="chart-container" position: relative; height:350px; width:100%">
      <canvas id="Chart2" width="400" height="400"></canvas>
  </div>
  <br>
  <div>
    <table id="dataTable">
      <tr>
        <th><i class="far fa-clock"></i> Time</th>
        <th><i class="fas fa-head-side-cough" style="color:#ffffff;"></i> CO2 concentration in ppm</th>
        <th><i class="fas fa-thermometer-half" style="color:#ffffff;"></i> Temperaure in &deg;C</th>
        <th><i class="fas fa-tint" style="color:#ffffff;"></i> Humidity in %</th>
      </tr>
    </table>
  </div>
<br>
<br>  

<script>

  // arrays for data values, will be dynamically filled
  // if length exceeds threshold, first (oldest) element is deleted
  var CO2values       = [];
  var Tvalues         = [];
  var Hvalues         = [];
  var timeStamp       = [];
  var maxArrayLength  = 1000;

  // update intervall for getting new data in milliseconds
  var updateIntervall = 10000;

  // Graphs visit: https://www.chartjs.org
  // graph for CO2 concentration
  var ctx = document.getElementById("Chart1").getContext('2d');
  var Chart1 = new Chart(ctx, {
    type: 'line',
    data: {
      labels: timeStamp,  //Bottom Labeling
      datasets: [{
        label           : "CO2 concentration",
        fill            : 'origin',                   // 'origin': fill area to x-axis
        backgroundColor : 'rgba( 243, 18, 156 , .5)', // point fill color
        borderColor     : 'rgba( 243, 18, 156 , 1)',  // point stroke color
        data            : CO2values,
      }],
    },
    options: {
      title: {
        display         : false,
        text            : "CO2 concentration"
      },
      maintainAspectRatio: false,
      elements: {
        line: {
          tension       : 0.5 //Smoothening (Curved) of data lines
        }
      },
      scales: {
        yAxes: [{
          display       : true,
          position      : 'left',
          ticks: {
            beginAtZero :false,
            precision   : 0,
            fontSize    :16
          },
          scaleLabel: {
            display     : true,
            // unicode for subscript:   u+208x, 
            //         for superscript: u+207x
            labelString : 'CO\u2082 in ppm',
            fontSize    : 20
          },
        }]
      }
    }
  });
  // temperature and humidity graph
  var ctx2 = document.getElementById("Chart2").getContext('2d');
  var Chart2 = new Chart(ctx2, {
    type: 'line',
    data: {
      labels: timeStamp,  //Bottom Labeling
      datasets: [{
        label           : "Temperature",
        fill            : false,                      // fill area to xAxis
        backgroundColor : 'rgba( 243, 156, 18 , 1)',  // marker color
        borderColor     : 'rgba( 243, 156, 18 , 1)',  // line Color
        yAxisID         : 'left',
        data            : Tvalues,
      }, {
        label           : "Humidity",
        fill            : false,                      // fill area to xAxis
        backgroundColor : 'rgba(104, 145, 195, 1)',   // marker color
        borderColor     : 'rgba(104, 145, 195, 1)',   // line Color
        data            : Hvalues,
        yAxisID         : 'right',
      }],
    },
    options: {
      title: {
        display         : false,
        text            : "CO2 Monitor"
      },
      maintainAspectRatio: false,
      elements: {
        line: {
          tension       : 0.4                         // smoothening (bezier curve tension)
        }
      },
      scales: {
        yAxes: [{
          id            : 'left',
          position      : 'left',
          scaleLabel: {
            display     : true,
            labelString : 'Temperature in \u00B0C',
            fontSize    : 20
          }, 
          ticks: {
            suggestedMin: 18,
            suggestedMax: 30,
            fontSize    : 16
          }
        }, {
          id            : 'right',
          position      : 'right',
          scaleLabel: {
            display     : true,
            labelString : 'Humidity in %',
            fontSize    : 20
          },
          ticks: {
            suggestedMin: 40,
            suggestedMax: 70,
            fontSize    : 16
          }
        }]
      }
    }
  });

  // function to dynamically updating graphs
  // much more efficient than replotting every time
  function updateCharts() {
    // update datasets to be plotted
    Chart1.data.datasets[0].data = CO2values;
    Chart2.data.datasets[0].data = Tvalues;
    Chart2.data.datasets[1].data = Hvalues;
    // update the charts
    Chart1.update();
    Chart2.update();
  };

  // function to download data arrays into csv-file
  function downloadData() {
    // build array of strings with data to be saved
    var data = [];
    for ( var ii=0 ; ii < CO2values.length ; ii++ ){
      data.push( [ timeStamp[ii], 
                   Math.round(CO2values[ii]).toString(), 
                   Tvalues[ii].toString(), 
                   Hvalues[ii].toString()
                 ]);
    }
    
    // build String containing all data to be saved (csv-formatted)
    var csv = 'Time,CO2 in ppm,Temperature in Celsius,Humidity in percent\n';
    data.forEach(function(row) {
      csv += row.join(',');
      csv += "\n";
    });
    
    // save csv-string into file
    // create a hyperlink element (defined by <a> tag)
    var hiddenElement     = document.createElement('a');
    // similar functions: encodeURI(), encodeURIComponent() (escape() not recommended)
    hiddenElement.href    = 'data:text/csv;charset=utf-8,'+encodeURI(csv);
    hiddenElement.target  = '_blank';
    hiddenElement.download= 'CO2monitor.csv';
    hiddenElement.click();
  };

  // ajax script to get data repetivitely
  
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
        CO2values.push(obj.COO);
        Tvalues.push(obj.Temperature);
        Hvalues.push(obj.Humidity);
        timeStamp.push(time);

        // if array becomes too long, delete oldest element to not overload graph
        // also delete first row of data table
        if (CO2values.length > maxArrayLength) {
          // shift() method to delete first element
          CO2values.shift();
          Tvalues.shift();
          Hvalues.shift();
          timeStamp.shift();
          
          // HTMLTableElement.deleteRow(index), index=-1 for last element
          document.getElementById("dataTable").deleteRow(-1);
        }
  
        // update graphs
        updateCharts();
        
        // update data table
        var table       = document.getElementById("dataTable");
        var row         = table.insertRow(1); //Add after headings
        var cell1       = row.insertCell(0);
        var cell2       = row.insertCell(1);
        var cell3       = row.insertCell(2);
        var cell4       = row.insertCell(3);
        cell1.innerHTML = time;
        cell2.innerHTML = Math.round(obj.COO);
        cell3.innerHTML = obj.Temperature;
        cell4.innerHTML = obj.Humidity;
      }
    };
    xhttp.open("GET", "readData", true); //Handle readData server on ESP8266
    xhttp.send();
  }

</script>
</body>
</html>
)=====";
