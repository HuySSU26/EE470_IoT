<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

// ============================================
// API KEY AUTHENTICATION (Only for DELETE)
// ============================================
define('VALID_API_KEY', 'my_secret_key');

function checkAuthentication() {
    $headers = getallheaders();
    $api_key = isset($headers['X-API-Key']) ? $headers['X-API-Key'] : null;
    
    $auth_header = isset($headers['Authorization']) ? $headers['Authorization'] : null;
    if ($auth_header && strpos($auth_header, 'Bearer ') === 0) {
        $bearer_token = substr($auth_header, 7);
    } else {
        $bearer_token = null;
    }
    
    return ($api_key === VALID_API_KEY || $bearer_token === VALID_API_KEY);
}

// ============================================
// Database credentials
// ============================================
$host = "localhost";
$username = "u411050800_db_SensorsData";
$password = "my_passcode";  
$database = "u411050800_SensorsData";

$conn = new mysqli($host, $username, $password, $database);

if ($conn->connect_error) {
    die(json_encode(["status" => "error", "message" => "Connection failed: " . $conn->connect_error]));
}

// ============================================
// Get Request Method and Input
// ============================================
$request_method = $_SERVER['REQUEST_METHOD'];

if ($request_method === 'POST') {
    $input = $_POST;
} elseif ($request_method === 'PUT' || $request_method === 'PATCH') {
    parse_str(file_get_contents('php://input'), $input);
} elseif ($request_method === 'DELETE') {
    parse_str(file_get_contents('php://input'), $input);
    if (empty($input)) {
        $input = $_GET;
    }
} else {
    $input = $_GET;
}

// ============================================
// Handle DELETE Request (REQUIRES AUTH)
// ============================================
if ($request_method === 'DELETE') {
    
    if (isset($input['node_name']) && isset($input['time_received'])) {
        
        if (!checkAuthentication()) {
            http_response_code(401);
            die(json_encode([
                "status" => "error", 
                "message" => "Unauthorized: DELETE requires API key"
            ]));
        }
        
        $node_name = $input['node_name'];
        $time_received = $input['time_received'];
        
        $delete_stmt = $conn->prepare("DELETE FROM sensor_data WHERE node_name = ? AND time_received = ?");
        $delete_stmt->bind_param("ss", $node_name, $time_received);
        
        if ($delete_stmt->execute()) {
            if ($delete_stmt->affected_rows > 0) {
                echo json_encode([
                    "status" => "success",
                    "message" => "Data deleted successfully",
                    "deleted" => [
                        "node_name" => $node_name,
                        "time_received" => $time_received
                    ]
                ]);
            } else {
                echo json_encode([
                    "status" => "error",
                    "message" => "No matching record found to delete"
                ]);
            }
        } else {
            echo json_encode(["status" => "error", "message" => "Delete failed: " . $delete_stmt->error]);
        }
        
        $delete_stmt->close();
        $conn->close();
        exit();
    } else {
        echo json_encode([
            "status" => "error",
            "message" => "DELETE requires both node_name and time_received parameters"
        ]);
        $conn->close();
        exit();
    }
}

// ============================================
// Handle Data Insertion (NO AUTH REQUIRED)
// ============================================
if (isset($input['node_name']) && (isset($input['temperature']) || isset($input['humidity']))) {
    
    $node_name = $input['node_name'];
    $temperature = isset($input['temperature']) && $input['temperature'] !== '' ? floatval($input['temperature']) : null;
    $humidity = isset($input['humidity']) && $input['humidity'] !== '' ? floatval($input['humidity']) : null;
    
    if (isset($input['time_received']) && !empty($input['time_received'])) {
        $time_received = $input['time_received'];
    } else {
        $time_received = date('Y-m-d H:i:s');
    }
    
    // Validate data ranges
    $errors = [];
    
    if ($temperature === null && $humidity === null) {
        $errors[] = "At least temperature or humidity must be provided";
    }
    
    if ($temperature !== null && ($temperature < -10 || $temperature > 100)) {
        $errors[] = "Temperature must be between -10 and 100°C";
    }
    
    if ($humidity !== null && ($humidity < 0 || $humidity > 100)) {
        $errors[] = "Humidity must be between 0 and 100%";
    }
    
    if (!empty($errors)) {
        http_response_code(400);
        echo json_encode([
            "status" => "error", 
            "message" => implode(", ", $errors)
        ]);
        $conn->close();
        exit();
    }
    
    // Check if node is registered
    $check_node = $conn->prepare("SELECT node_name FROM sensor_register WHERE node_name = ?");
    $check_node->bind_param("s", $node_name);
    $check_node->execute();
    $result = $check_node->get_result();
    
    if ($result->num_rows == 0) {
        echo json_encode(["status" => "error", "message" => "Node '$node_name' is not registered"]);
        $check_node->close();
        $conn->close();
        exit();
    }
    $check_node->close();
    
    // Check for duplicate time for same node
    $check_duplicate = $conn->prepare("SELECT * FROM sensor_data WHERE node_name = ? AND time_received = ?");
    $check_duplicate->bind_param("ss", $node_name, $time_received);
    $check_duplicate->execute();
    $dup_result = $check_duplicate->get_result();
    
    if ($dup_result->num_rows > 0) {
        echo json_encode(["status" => "error", "message" => "Duplicate entry: Data for node '$node_name' at time '$time_received' already exists"]);
        $check_duplicate->close();
        $conn->close();
        exit();
    }
    $check_duplicate->close();
    
    // Insert data
    $insert = $conn->prepare("INSERT INTO sensor_data (node_name, time_received, temperature, humidity) VALUES (?, ?, ?, ?)");
    
    if (!$insert) {
        echo json_encode(["status" => "error", "message" => "Prepare failed: " . $conn->error]);
        $conn->close();
        exit();
    }
    
    $insert->bind_param("ssdd", $node_name, $time_received, $temperature, $humidity);
    
    if ($insert->execute()) {
        echo json_encode([
            "status" => "success", 
            "message" => "Data inserted successfully via " . $request_method,
            "data" => [
                "node_name" => $node_name,
                "time_received" => $time_received,
                "temperature" => $temperature,
                "humidity" => $humidity
            ]
        ]);
    } else {
        echo json_encode([
            "status" => "error", 
            "message" => "Insert failed: " . $insert->error
        ]);
    }
    $insert->close();
    $conn->close();
    exit();
}

// ============================================
// Display Simple Dashboard (No authentication needed)
// ============================================
?>
<!DOCTYPE html>
<html>
<head>
    <title>SSU IoT Lab - Data Management Dashboard</title>
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
        .info-box {
            background-color: #e6f3ff;
            padding: 20px;
            border-radius: 8px;
            margin: 20px 0;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .info-box h3 {
            margin-top: 0;
            color: #003366;
        }
        .info-box a {
            display: inline-block;
            margin: 10px 10px 0 0;
            padding: 10px 20px;
            background-color: #0066cc;
            color: white;
            text-decoration: none;
            border-radius: 4px;
        }
        .info-box a:hover {
            background-color: #0052a3;
        }
        .info-box a.chart-link {
            background-color: #28a745;
        }
        .info-box a.chart-link:hover {
            background-color: #218838;
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
        .node-selector {
            margin: 20px 0;
            padding: 15px;
            background-color: #f0f0f0;
            border-radius: 5px;
        }
        .node-selector label {
            font-weight: bold;
            margin-right: 10px;
        }
        .node-selector select {
            padding: 8px;
            font-size: 14px;
            border: 1px solid #ccc;
            border-radius: 4px;
        }
        .node-selector button {
            padding: 8px 20px;
            margin-left: 10px;
            background-color: #0066cc;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-size: 14px;
        }
        .node-selector button:hover {
            background-color: #0052a3;
        }
        .average-display {
            background-color: #ffffcc;
            padding: 15px;
            margin: 20px 0;
            border-radius: 5px;
            font-size: 16px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>SSU IoT Lab - Data Management Dashboard</h1>
        
        <!-- Quick Links -->
        <div class="info-box">
            <h3>📊 Visualization & Data Access</h3>
            <p>Use these links to visualize and access your sensor data:</p>
            <a href="graph.html" class="chart-link" target="_blank">📈 View Interactive Charts</a>
            <a href="data.php" target="_blank">📄 Get JSON Data (All Nodes)</a>
            
            <!-- Node Selector for JSON -->
            <div style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #ccc;">
                <label for="jsonNodeSelect" style="font-weight: bold; margin-right: 10px;">Get JSON Data for Specific Node:</label>
                <select id="jsonNodeSelect" style="padding: 8px; font-size: 14px; border: 1px solid #ccc; border-radius: 4px;">
                    <?php
                    $json_nodes_query = "SELECT DISTINCT node_name FROM sensor_register ORDER BY node_name";
                    $json_nodes_result = $conn->query($json_nodes_query);
                    
                    if ($json_nodes_result && $json_nodes_result->num_rows > 0) {
                        while($node_row = $json_nodes_result->fetch_assoc()) {
                            $node = $node_row['node_name'];
                            echo '<option value="' . htmlspecialchars($node) . '">' . htmlspecialchars($node) . '</option>';
                        }
                    }
                    ?>
                </select>
                <button onclick="viewJsonData()" style="padding: 8px 20px; margin-left: 10px; background-color: #17a2b8; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 14px;">🔍 View JSON</button>
            </div>
        </div>
        
        <script>
        function viewJsonData() {
            var selectedNode = document.getElementById('jsonNodeSelect').value;
            window.open('data.php?node=' + selectedNode, '_blank');
        }
        </script>

<?php
// Display Registered Sensor Nodes
echo '<h2>Registered Sensor Nodes</h2>';
$query1 = "SELECT node_name, manufacturer, longitude, latitude FROM sensor_register ORDER BY node_name";
$result1 = $conn->query($query1);

if ($result1 && $result1->num_rows > 0) {
    echo '<table><tr><th>Node Name</th><th>Manufacturer</th><th>Longitude</th><th>Latitude</th></tr>';
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

echo '<h2>Data Received</h2>';

// Node selector for averages
echo '<div class="node-selector">';
echo '<form method="GET" action="">';
echo '<label for="node_select">Select Node for Average:</label>';
echo '<select name="selected_node" id="node_select">';

$selected_node = isset($_GET['selected_node']) ? $_GET['selected_node'] : 'node_1';

$query_nodes = "SELECT DISTINCT node_name FROM sensor_register ORDER BY node_name";
$result_nodes = $conn->query($query_nodes);

if ($result_nodes && $result_nodes->num_rows > 0) {
    while($node_row = $result_nodes->fetch_assoc()) {
        $node = $node_row['node_name'];
        $selected = ($node == $selected_node) ? 'selected' : '';
        echo '<option value="' . htmlspecialchars($node) . '" ' . $selected . '>' . htmlspecialchars($node) . '</option>';
    }
}

echo '</select>';
echo '<button type="submit">Show Average</button>';
echo '</form>';
echo '</div>';

// Calculate and Display Average for selected node
$query3 = "SELECT AVG(temperature) as avg_temp, AVG(humidity) as avg_humidity 
           FROM sensor_data 
           WHERE node_name = ?";
$stmt = $conn->prepare($query3);
$stmt->bind_param("s", $selected_node);
$stmt->execute();
$result3 = $stmt->get_result();

if ($result3 && $result3->num_rows > 0) {
    $row = $result3->fetch_assoc();
    if ($row['avg_temp'] !== null) {
        echo '<div class="average-display">';
        echo '<strong>The Average Temperature for ' . htmlspecialchars($selected_node) . ' has been: ' . number_format($row['avg_temp'], 0) . '°C</strong><br>';
        echo '<strong>The Average Humidity for ' . htmlspecialchars($selected_node) . ' has been: ' . number_format($row['avg_humidity'], 0) . '%</strong>';
        echo '</div>';
    }
}
$stmt->close();

// Display the data table
$query2 = "SELECT node_name, time_received, temperature, humidity FROM sensor_data ORDER BY node_name, time_received DESC LIMIT 100";
$result2 = $conn->query($query2);

if ($result2 && $result2->num_rows > 0) {
    echo '<table><tr><th>Node Name</th><th>Time Received</th><th>Temperature (°C)</th><th>Humidity (%)</th></tr>';
    while($row = $result2->fetch_assoc()) {
        echo '<tr>';
        echo '<td>' . htmlspecialchars($row['node_name']) . '</td>';
        echo '<td>' . htmlspecialchars($row['time_received']) . '</td>';
        echo '<td>' . htmlspecialchars($row['temperature']) . '</td>';
        echo '<td>' . htmlspecialchars($row['humidity']) . '</td>';
        echo '</tr>';
    }
    echo '</table>';
    echo '<p style="text-align: center; color: #666;"><em>Showing last 100 records. For charts and full data, use the links above.</em></p>';
} else {
    echo '<p>No sensor data found.</p>';
}

$conn->close();
?>

    </div>
</body>
</html>


