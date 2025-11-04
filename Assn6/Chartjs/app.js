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
      
      var x_axis = []; // Time labels for display
      var y_axis = []; // Temperature values
      var full_timestamps = []; // Store full timestamps for tooltip

      for(var i in data) {
        // Store full timestamp for tooltip
        var timestamp = data[i].time_received;
        full_timestamps.push(timestamp);
        
        /*// Extract just time portion for x-axis display
        var timeOnly = timestamp.split(' ')[1]; // Get HH:MM:SS
        x_axis.push(timeOnly); // only display time_received on x-axis*/
		
		x_axis.push(timestamp);  // Display full date & time on x-axis
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
            pointHoverRadius: 6,
            // Store full timestamps in the dataset for tooltip access
            fullTimestamps: full_timestamps
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
                // Show full date and time in tooltip title
                title: function(context) {
                  var index = context[0].dataIndex;
                  var fullTimestamp = context[0].dataset.fullTimestamps[index];
                  return fullTimestamp; // Shows: YYYY-MM-DD HH:MM:SS
                },
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