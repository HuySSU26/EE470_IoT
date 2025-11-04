<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

// ============================================
// IFTTT WEBHOOK CONFIGURATION
// ============================================
define('IFTTT_WEBHOOK_KEY', 'WEBHOOK_KEY');  // Replace with  actual key
define('IFTTT_EVENT_NAME', 'sensor_alert');
define('ENABLE_IFTTT_NOTIFICATIONS', true);  // Set to false to disable notifications

// ============================================
// API KEY AUTHENTICATION (Only for DELETE)
// ============================================
define('VALID_API_KEY', 'my_secrete_key');

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
// IFTTT NOTIFICATION FUNCTION
// ============================================
function sendIFTTTNotification($node_name, $temperature, $humidity) {
    if (!ENABLE_IFTTT_NOTIFICATIONS) {
        return false;
    }
    
    $webhook_url = "https://maker.ifttt.com/trigger/" . IFTTT_EVENT_NAME . "/with/key/" . IFTTT_WEBHOOK_KEY;
    
    $data = array(
        'value1' => $node_name,
        'value2' => number_format($temperature, 1),
        'value3' => number_format($humidity, 1)
    );
    
    $options = array(
        'http' => array(
            'header'  => "Content-type: application/json\r\n",
            'method'  => 'POST',
            'content' => json_encode($data),
            'timeout' => 5
        )
    );
    
    $context  = stream_context_create($options);
    $result = @file_get_contents($webhook_url, false, $context);
    
    if ($result === FALSE) {
        error_log("IFTTT notification failed for $node_name");
        return false;
    }
    
    error_log("IFTTT notification sent successfully for $node_name");
    return true;
}

// ============================================
// Database credentials
// ============================================
$host = "localhost";
$username = "u411050800_db_SensorsData";
$password = "my_passcode";   // Replace with  actual PSK 
$database = "u411050800_SensorsData";

$conn = new mysqli($host, $username, $password, $database);

if ($conn->connect_error) {
    http_response_code(500);
    die(json_encode(["status" => "error", "message" => "Connection failed: " . $conn->connect_error]));
}

// ============================================
// Get Request Method and Input
// ============================================
$request_method = $_SERVER['REQUEST_METHOD'];

// Handle JSON input from ESP8266
$json_input = null;
if ($request_method === 'POST' || $request_method === 'PUT') {
    $raw_input = file_get_contents('php://input');
    if (!empty($raw_input)) {
        $json_input = json_decode($raw_input, true);
    }
}

if ($json_input !== null) {
    $input = $json_input;
} elseif ($request_method === 'POST') {
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
// Handle ESP8266 Data Insertion (JSON format)
// ============================================
if (isset($input['node']) && isset($input['timestamp']) && 
    (isset($input['temperature_C']) || isset($input['humidity_pct']))) {
    
    // ESP8266 sends: node (int), temperature_C, humidity_pct, timestamp, activity_count
    $node_number = intval($input['node']);
    $node_name = 'node_' . $node_number;
    
    $temperature = isset($input['temperature_C']) && $input['temperature_C'] !== '' 
        ? floatval($input['temperature_C']) : null;
    $humidity = isset($input['humidity_pct']) && $input['humidity_pct'] !== '' 
        ? floatval($input['humidity_pct']) : null;
    $timestamp = $input['timestamp'];
    $activity_count = isset($input['activity_count']) ? intval($input['activity_count']) : 0;
    
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
    
    // Validate timestamp format (ISO 8601)
    if (empty($timestamp)) {
        $errors[] = "Timestamp is required";
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
    
    // Convert ISO 8601 timestamp to MySQL format (YYYY-MM-DD HH:MM:SS)
    try {
        $dt = new DateTime($timestamp);
        $mysql_timestamp = $dt->format('Y-m-d H:i:s');
    } catch (Exception $e) {
        http_response_code(400);
        echo json_encode([
            "status" => "error", 
            "message" => "Invalid timestamp format: " . $e->getMessage()
        ]);
        $conn->close();
        exit();
    }
    
    // Check if node is registered (auto-register if not exists)
    $check_node = $conn->prepare("SELECT node_name FROM sensor_register WHERE node_name = ?");
    $check_node->bind_param("s", $node_name);
    $check_node->execute();
    $result = $check_node->get_result();
    
    if ($result->num_rows == 0) {
        // Auto-register the node
        $register_stmt = $conn->prepare("INSERT INTO sensor_register (node_name, manufacturer, longitude, latitude) VALUES (?, 'ESP8266', 0.0, 0.0)");
        $register_stmt->bind_param("s", $node_name);
        $register_stmt->execute();
        $register_stmt->close();
    }
    $check_node->close();
    
    // Check for duplicate timestamp for same node
    $check_duplicate = $conn->prepare("SELECT * FROM sensor_data WHERE node_name = ? AND time_received = ?");
    $check_duplicate->bind_param("ss", $node_name, $mysql_timestamp);
    $check_duplicate->execute();
    $dup_result = $check_duplicate->get_result();
    
    if ($dup_result->num_rows > 0) {
        http_response_code(409);
        echo json_encode([
            "status" => "error", 
            "message" => "Duplicate entry: Data for this node and timestamp already exists",
            "node_name" => $node_name,
            "timestamp" => $mysql_timestamp
        ]);
        $check_duplicate->close();
        $conn->close();
        exit();
    }
    $check_duplicate->close();
    
    // Insert sensor data
    $insert_stmt = $conn->prepare("INSERT INTO sensor_data (node_name, time_received, temperature, humidity) VALUES (?, ?, ?, ?)");
    $insert_stmt->bind_param("ssdd", $node_name, $mysql_timestamp, $temperature, $humidity);
    
    if ($insert_stmt->execute()) {
        
        // ============================================
        // SEND IFTTT NOTIFICATION
        // ============================================
        $ifttt_sent = sendIFTTTNotification($node_name, $temperature, $humidity);
        
        // Update activity counter
        $check_activity = $conn->prepare("SELECT node_name FROM sensor_activity WHERE node_name = ?");
        $check_activity->bind_param("s", $node_name);
        $check_activity->execute();
        $activity_result = $check_activity->get_result();
        
        if ($activity_result->num_rows > 0) {
            $update_activity = $conn->prepare("UPDATE sensor_activity SET activity_count = activity_count + ?, last_update = NOW() WHERE node_name = ?");
            $update_activity->bind_param("is", $activity_count, $node_name);
            $update_activity->execute();
            $update_activity->close();
        } else {
            $insert_activity = $conn->prepare("INSERT INTO sensor_activity (node_name, activity_count, last_update) VALUES (?, ?, NOW())");
            $insert_activity->bind_param("si", $node_name, $activity_count);
            $insert_activity->execute();
            $insert_activity->close();
        }
        $check_activity->close();
        
        echo json_encode([
            "status" => "success",
            "message" => "Data inserted successfully",
            "ifttt_notification" => $ifttt_sent ? "sent" : "disabled or failed",
            "data" => [
                "node_name" => $node_name,
                "timestamp" => $mysql_timestamp,
                "temperature" => $temperature,
                "humidity" => $humidity
            ]
        ]);
    } else {
        echo json_encode([
            "status" => "error",
            "message" => "Insert failed: " . $insert_stmt->error
        ]);
    }
    
    $insert_stmt->close();
    $conn->close();
    exit();
}

// ============================================
// If we get here, it's a GET request - display the dashboard
// ============================================
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SSU IoT Lab - Sensor Dashboard</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        h1, h2 {
            color: #333;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 20px 0;
        }
        th, td {
            padding: 12px;
            text-align: left;
            border-bottom: 1px solid #ddd;
        }
        th {
            background-color: #4CAF50;
            color: white;
        }
        tr:hover {
            background-color: #f5f5f5;
        }
        .info-box {
            background-color: #e7f3fe;
            border-left: 6px solid #2196F3;
            padding: 15px;
            margin: 20px 0;
            border-radius: 4px;
        }
        .chart-link {
            display: inline-block;
            margin: 10px 10px 10px 0;
            padding: 10px 20px;
            background-color: #2196F3;
            color: white;
            text-decoration: none;
            border-radius: 4px;
            transition: background-color 0.3s;
        }
        .chart-link:hover {
            background-color: #0b7dda;
        }
        .ifttt-status {
            background-color: #fff3cd;
            border-left: 6px solid #ffc107;
            padding: 15px;
            margin: 20px 0;
            border-radius: 4px;
        }
        .ifttt-enabled {
            background-color: #d4edda;
            border-left: 6px solid #28a745;
        }
        .activity-table {
            background-color: #e8f5e9;
        }
        .average-display {
            background-color: #ffffcc;
            padding: 15px;
            margin: 20px 0;
            border-radius: 5px;
            font-size: 16px;
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
    </style>
</head>
<body>
    <div class="container">
        <h1>🌡️ SSU IoT Lab - Sensor Dashboard</h1>
        
        <!-- IFTTT Status -->
        <div class="<?php echo ENABLE_IFTTT_NOTIFICATIONS ? 'ifttt-status ifttt-enabled' : 'ifttt-status'; ?>">
            <h3>📱 IFTTT Notifications</h3>
            <p><strong>Status:</strong> <?php echo ENABLE_IFTTT_NOTIFICATIONS ? '✅ ENABLED' : '❌ DISABLED'; ?></p>
            <?php if (ENABLE_IFTTT_NOTIFICATIONS): ?>
            <p><strong>Event:</strong> <?php echo IFTTT_EVENT_NAME; ?></p>
            <p><strong>Notifications sent to:</strong></p>
            <ul>
                <li>📱 SMS: +1 707-547-7017</li>
                <li>💬 Slack: #iot-sensors channel</li>
            </ul>
            <?php endif; ?>
        </div>
        
        <!-- Quick Links -->
        <div class="info-box">
            <h3>📊 Visualization & Data Access</h3>
            <p>Use these links to visualize and access your sensor data:</p>
            <a href="graph.html" class="chart-link" target="_blank">📈 View Interactive Charts</a>
            <a href="data.php" target="_blank">📄 Get JSON Data (All Nodes)</a>
            <a href="googleview.html" target="_blank">🎨 Google Dashboard</a>
            <a href="googleview_sheets.html" target="_blank">📊 Google Sheets Dashboard</a>
            
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
// Display Activity Counter
echo '<h2>📱 Node Activity Counters</h2>';
$query_activity = "SELECT node_name, activity_count, last_update FROM sensor_activity ORDER BY node_name";
$result_activity = $conn->query($query_activity);

if ($result_activity && $result_activity->num_rows > 0) {
    echo '<table class="activity-table"><tr><th>Node Name</th><th>Total Activities</th><th>Last Activity</th></tr>';
    while($row = $result_activity->fetch_assoc()) {
        echo '<tr>';
        echo '<td>' . htmlspecialchars($row['node_name']) . '</td>';
        echo '<td><strong>' . htmlspecialchars($row['activity_count']) . '</strong></td>';
        echo '<td>' . htmlspecialchars($row['last_update']) . '</td>';
        echo '</tr>';
    }
    echo '</table>';
} else {
    echo '<div style="background-color: #fff3cd; padding: 15px; border-radius: 5px; border-left: 4px solid #ffc107;">';
    echo '<p><strong>⚠️ No activity data found.</strong></p>';
    echo '<p>This is normal if you haven\'t sent data from your ESP8266 yet.</p>';
    echo '</div>';
}

// Display Registered Sensor Nodes
echo '<h2>📍 Registered Sensor Nodes</h2>';
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

echo '<h2>🌡️ Sensor Data</h2>';

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
        echo '<strong>📊 The Average Temperature for ' . htmlspecialchars($selected_node) . ' has been: ' . number_format($row['avg_temp'], 1) . '°C</strong><br>';
        echo '<strong>💧 The Average Humidity for ' . htmlspecialchars($selected_node) . ' has been: ' . number_format($row['avg_humidity'], 1) . '%</strong>';
        echo '</div>';
    }
}
$stmt->close();

// Display the data table - sorted by time_received DESC (newest first)
$query2 = "SELECT node_name, time_received, temperature, humidity 
           FROM sensor_data 
           ORDER BY node_name ASC, time_received DESC 
           LIMIT 100";
$result2 = $conn->query($query2);

if ($result2 && $result2->num_rows > 0) {
    echo '<table><tr><th>Node Name</th><th>Time Received</th><th>Temperature (°C)</th><th>Humidity (%)</th></tr>';
    while($row = $result2->fetch_assoc()) {
        echo '<tr>';
        echo '<td>' . htmlspecialchars($row['node_name']) . '</td>';
        echo '<td>' . htmlspecialchars($row['time_received']) . '</td>';
        echo '<td>' . number_format($row['temperature'], 1) . '</td>';
        echo '<td>' . number_format($row['humidity'], 1) . '</td>';
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
