<?php
/**
 * Sensor Data API Endpoint for Google Dashboard
 * Returns sensor data in JSON format
 */

// Enable CORS for JavaScript to access this endpoint
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET');
header('Access-Control-Allow-Headers: Content-Type');
header('Content-Type: application/json');

// Prevent caching - IMPORTANT for real-time data!
header('Cache-Control: no-cache, no-store, must-revalidate');
header('Pragma: no-cache');
header('Expires: 0');

// Database configuration - FROM YOUR sensor_dashboard.php
$servername = "localhost";
$username = "u411050800_db_SensorsData";
$password = "my_passcode";
$dbname = "u411050800_SensorsData";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    http_response_code(500);
    echo json_encode([
        'error' => true,
        'message' => 'Database connection failed: ' . $conn->connect_error
    ]);
    exit;
}

// Get optional parameters
$limit = isset($_GET['limit']) ? intval($_GET['limit']) : 100;
$order = isset($_GET['order']) && $_GET['order'] === 'asc' ? 'ASC' : 'DESC'; // Default to DESC (newest first)
$data_type = isset($_GET['type']) ? $_GET['type'] : 'temperature'; // 'temperature' or 'humidity'
$node = isset($_GET['node']) ? $_GET['node'] : null; // Optional: filter by specific node

// Choose which sensor data to display
if ($data_type === 'humidity') {
    $value_column = 'humidity';
} else {
    $value_column = 'temperature';
}

// Build SQL query
if ($node !== null) {
    // Filter by specific node
    $sql = "SELECT $value_column as sensor_value, time_received as timestamp, node_name 
            FROM sensor_data 
            WHERE node_name = ?
            ORDER BY time_received $order 
            LIMIT ?";
    $stmt = $conn->prepare($sql);
    $stmt->bind_param("si", $node, $limit);
} else {
    // All nodes
    $sql = "SELECT $value_column as sensor_value, time_received as timestamp, node_name 
            FROM sensor_data 
            ORDER BY time_received $order 
            LIMIT ?";
    $stmt = $conn->prepare($sql);
    $stmt->bind_param("i", $limit);
}

$stmt->execute();
$result = $stmt->get_result();

$data = [];
if ($result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        $data[] = [
            'sensor_value' => floatval($row['sensor_value']),
            'timestamp' => $row['timestamp'],
            'node_name' => $row['node_name']
        ];
    }
}

// Return JSON response
echo json_encode($data, JSON_PRETTY_PRINT);

$stmt->close();
$conn->close();
?>