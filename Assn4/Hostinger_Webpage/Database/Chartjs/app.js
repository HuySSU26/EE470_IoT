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
            label: 'Temperature (\u00B0C)',
            // GREEN COLORS - Change these for different colors
            backgroundColor: 'rgba(75, 192, 75, 0.2)',
            borderColor: 'rgba(75, 192, 75, 1)',
            borderWidth: 2,
            fill: true,
            tension: 0.4,
            data: y_axis,
            pointRadius: 4,
            pointHoverRadius: 6
          }
        ]
      };

      var ctx = $("#mycanvas");

      var lineGraph = new Chart(ctx, {
        type: 'line',
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
                text: 'Temperature (\u00B0C)'
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
              text: 'Sensor ' + chartNode
            },
            tooltip: {
              enabled: true,
              callbacks: {
                label: function(context) {
                  return 'Temperature: ' + context.parsed.y.toFixed(2) + '\u00B0C';
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
});
