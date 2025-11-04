<?php
header('Content-Type: application/json');

// Database credentials
$host = "localhost";
$username = "u411050800_db_SensorsData";
$password = "my_passcode";  
$database = "u411050800_SensorsData";

$conn = new mysqli($host, $username, $password, $database);

if ($conn->connect_error) {
    die(json_encode(["error" => "Connection failed: " . $conn->connect_error]));
}

// Query to get all registered nodes
$query = "SELECT node_name FROM sensor_register ORDER BY node_name ASC";
$result = $conn->query($query);

$nodes = [];

if ($result && $result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        $nodes[] = $row['node_name'];
    }
}

// Return array of node names
echo json_encode($nodes);

$conn->close();
?>