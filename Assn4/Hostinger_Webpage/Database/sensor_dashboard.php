<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

// Database credentials
$host = "localhost";
$username = "u411050800_db_SensorsData";
$password = "8EG15mp8317^";  
$database = "u411050800_SensorsData";
?>

<!DOCTYPE html>
<html>
<head>
    <title>SSU IoT Lab - Sensor Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            max-width: 1200px;
            margin: 20px auto;
            padding: 20px;
            background-color: #f5f5f5;
        }
        h1 {
            text-align: center;
            color: #003366;
            margin-bottom: 10px;
        }
        h2 {
            color: #0066cc;
            margin-top: 30px;
            margin-bottom: 15px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            background-color: white;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
            margin-bottom: 30px;
        }
        th {
            background-color: #9ACD32;
            color: white;
            padding: 12px;
            text-align: left;
            font-weight: bold;
        }
        td {
            padding: 10px 12px;
            border-bottom: 1px solid #ddd;
        }
        tr:nth-child(even) {
            background-color: #f9f9f9;
        }
        tr:hover {
            background-color: #f0f0f0;
        }
        .container {
            background-color: white;
            padding: 20px;
            border-radius: 8px;
        }
        .error {
            color: red;
            padding: 10px;
            background-color: #ffe6e6;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Welcome to SSU IoT Lab</h1>

<?php
// Database credentials
$host = "localhost";
$username = "u411050800_db_SensorsData";
$password = "8EG15mp8317^"; 
$database = "u411050800_SensorsData";

// Create connection
$conn = new mysqli($host, $username, $password, $database);

// Check connection
if ($conn->connect_error) {
    echo '<p class="error">Connection failed: ' . $conn->connect_error . '</p>';
    exit();
}

// Display Registered Sensor Nodes
echo '<h2>Registered Sensor Nodes</h2>';
$query1 = "SELECT node_name, manufacturer, longitude, latitude 
           FROM sensor_register 
           ORDER BY node_name";
$result1 = $conn->query($query1);

if ($result1 && $result1->num_rows > 0) {
    echo '<table>';
    echo '<tr>
            <th>Node Name</th>
            <th>Manufacturer</th>
            <th>Longitude</th>
            <th>Latitude</th>
          </tr>';
    
    while($row = $result1->fetch_assoc()) {
        echo '<tr>';
        echo '<td>' . htmlspecialchars($row['node_name']) . '</td>';
        echo '<td>' . htmlspecialchars($row['manufacturer']) . '</td>';
        echo '<td>' . htmlspecialchars($row['longitude']) . '</td>';
        echo '<td>' . htmlspecialchars($row['latitude']) . '</td>';
        echo '</tr>';
    }
    echo '</table>';
} else {
    echo '<p>No registered sensors found.</p>';
}

// Display Data Received
echo '<h2>Data Received</h2>';
$query2 = "SELECT node_name, time_received, temperature, humidity 
           FROM sensor_data 
           ORDER BY node_name, time_received";
$result2 = $conn->query($query2);

if ($result2 && $result2->num_rows > 0) {
    echo '<table>';
    echo '<tr>
            <th>Node Name</th>
            <th>Time Received</th>
            <th>Temperature (°C)</th>
            <th>Humidity (%)</th>
          </tr>';
    
    while($row = $result2->fetch_assoc()) {
        echo '<tr>';
        echo '<td>' . htmlspecialchars($row['node_name']) . '</td>';
        echo '<td>' . htmlspecialchars($row['time_received']) . '</td>';
        echo '<td>' . htmlspecialchars($row['temperature']) . '</td>';
        echo '<td>' . htmlspecialchars($row['humidity']) . '</td>';
        echo '</tr>';
    }
    echo '</table>';
} else {
    echo '<p>No sensor data found.</p>';
}

// Close connection
$conn->close();
?>

    </div>
</body>
</html>
