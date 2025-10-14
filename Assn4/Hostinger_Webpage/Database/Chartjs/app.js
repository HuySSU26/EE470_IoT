$(document).ready(function(){
  // IMPORTANT: Get node from URL parameter first, then fallback to window variable
  var urlParams = new URLSearchParams(window.location.search);
  var chartNode = urlParams.get('node') || window.selectedChartNode || 'node_1';
  
  console.log("Loading chart for node:", chartNode);
  
  $.ajax({
    url: "data.php?node=" + chartNode + "&t=" + new Date().getTime(),
    method: "GET",
    success: function(data) {
      console.log("Data received:", data);
      
      var x_axis = []; // Time labels
      var y_axis = []; // Temperature values

      for(var i in data) {
        // Extract just time portion from timestamp
        var timestamp = data[i].time_received;
        var timeOnly = timestamp.split(' ')[1]; // Get HH:MM:SS
        x_axis.push(timeOnly);
        y_axis.push(parseFloat(data[i].temperature));
      }

      var chartdata = {
        labels: x_axis,
        datasets: [
          {
            label: 'Temperature (°C)',  // Fixed character encoding
            // GREEN COLORS - Change these for different colors
            backgroundColor: 'rgba(75, 192, 75, 0.2)',
            borderColor: 'rgba(75, 192, 75, 1)',
            borderWidth: 2,
            fill: true,
            tension: 0.4,
            data: y_axis,
            pointRadius: 4,           // Make points visible
            pointHoverRadius: 6       // Larger on hover
          }
        ]
      };

      var ctx = $("#mycanvas");

      var lineGraph = new Chart(ctx, {
        type: 'line',   // Change to 'bar' for bar chart
        data: chartdata,
        options: {
          responsive: true,
          maintainAspectRatio: true,
          interaction: {
            mode: 'index',
            intersect: false
          },
          scales: {
            y: {
              beginAtZero: true,
              title: {
                display: true,
                text: 'Temperature (°C)'  // Fixed character encoding
              }
            },
            x: {
              title: {
                display: true,
                text: 'Time'
              }
            }
          },
          plugins: {
            legend: {
              display: true,
              position: 'top'
            },
            title: {
              display: true,
              text: 'Sensor ' + chartNode  // Dynamic title based on selected node
            },
            tooltip: {
              enabled: true,
              callbacks: {
                label: function(context) {
                  return 'Temperature: ' + context.parsed.y.toFixed(2) + '°C';
                }
              }
            }
          }
        }
      });
    },
    error: function(xhr, status, error) {
      console.log("Error loading chart data:", error);
      console.log("Status:", status);
      console.log("Response:", xhr.responseText);
    }
  });
});$(document).ready(function(){
  // Get the chart node from the page's data attribute or window variable
  // This will be set by sensor_dashboard.php
  var chartNode = window.selectedChartNode || 'node_1';  // Fallback to node_1 if not set
  
  $.ajax({
    url: "https://huynguyen.co/Chartjs/sensor_dashboard.php?format=json&node=" + chartNode,
    method: "GET",
    success: function(data) {
      console.log(data);
      
      var timeLabels = [];  // For x-axis (time)
      var temperatureData = [];  // For y-axis (temperature)

      for(var i in data) {
        // Extract just the time part (HH:MM:SS) from timestamp
        var timestamp = data[i].time_received;
        var timeOnly = timestamp.split(' ')[1]; // Get time portion
        timeLabels.push(timeOnly);
        
        // Get temperature value
        temperatureData.push(data[i].temperature);
      }

      var chartdata = {
        labels: timeLabels,
        datasets: [
          {
            label: 'Temperature (°C)', // Chart legend label
            
            // GREEN COLORS - Change these for different colors
            backgroundColor: 'rgba(75, 192, 75, 0.2)',  // Light green fill (transparent)
            borderColor: 'rgba(75, 192, 75, 1)',        // Dark green line (solid)
            
            // Line styling
            borderWidth: 2,           // Line thickness
            fill: true,               // Fill area under line
            tension: 0.4,             // Curve smoothness (0 = straight, 0.4 = smooth)
            
            data: temperatureData
          }
        ]
      };

      var ctx = $("#mycanvas");

      var lineGraph = new Chart(ctx, {
        type: 'line',   // CHANGED FROM 'bar' TO 'line'
        data: chartdata,
        options: {
          responsive: true,
          maintainAspectRatio: true,
          scales: {
            y: {
              beginAtZero: true,
              title: {
                display: true,
                text: 'Temperature (°C)'
              }
            },
            x: {
              title: {
                display: true,
                text: 'Time'
              }
            }
          },
          plugins: {
            legend: {
              display: true,
              position: 'top'
            },
            title: {
              display: true,
              text: 'Sensor ' + chartNode  // Chart title
            }
          }
        }
      });
    },
    error: function(data) {
      console.log("Error loading chart data:", data);
    }
  });
});