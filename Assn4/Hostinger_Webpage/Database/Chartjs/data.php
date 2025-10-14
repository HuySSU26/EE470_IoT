<?php
header('Content-Type: application/json');

// Database credentials
$host = "localhost";
$username = "u411050800_db_SensorsData";
$password = "8EG15mp8317^";  
$database = "u411050800_SensorsData";

$conn = new mysqli($host, $username, $password, $database);

if ($conn->connect_error) {
    die(json_encode(["error" => "Connection failed: " . $conn->connect_error]));
}

// Get node parameter (optional filter)
$node = isset($_GET['node']) ? $_GET['node'] : null;

// Build query
if ($node) {
    $query = "SELECT node_name, time_received, temperature, humidity 
              FROM sensor_data 
              WHERE node_name = ? 
              ORDER BY time_received ASC";
    $stmt = $conn->prepare($query);
    $stmt->bind_param("s", $node);
    $stmt->execute();
    $result = $stmt->get_result();
} else {
    $query = "SELECT node_name, time_received, temperature, humidity 
              FROM sensor_data 
              ORDER BY node_name, time_received ASC";
    $result = $conn->query($query);
}

// Fetch data
$data = [];
if ($result && $result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        $data[] = $row;
    }
}

// Return JSON
echo json_encode($data);

if (isset($stmt)) $stmt->close();
$conn->close();
?>