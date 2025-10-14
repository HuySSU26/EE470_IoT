<?php
error_reporting(E_ALL);
ini_set('display_errors', 1);

// ============================================
// API KEY AUTHENTICATION
// ============================================
// Generate your API key using: https://www.uuidgenerator.net/
// Or create a random string: bin2hex(random_bytes(32))
define('VALID_API_KEY', 'my_secret_passcode');

// Check for API Key in headers
$headers = getallheaders();
$api_key = isset($headers['X-API-Key']) ? $headers['X-API-Key'] : null;

// Alternative: Check Authorization header for Bearer token
$auth_header = isset($headers['Authorization']) ? $headers['Authorization'] : null;
if ($auth_header && strpos($auth_header, 'Bearer ') === 0) {
    $bearer_token = substr($auth_header, 7); // Remove "Bearer " prefix
} else {
    $bearer_token = null;
}

// Validate API Key or Bearer Token
if ($api_key !== VALID_API_KEY && $bearer_token !== VALID_API_KEY) {
    http_response_code(401); // Unauthorized
    die(json_encode([
        "status" => "error", 
        "message" => "Unauthorized: Invalid or missing API key/token"
    ]));
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

// Support both GET and POST methods
$request_method = $_SERVER['REQUEST_METHOD'];

// Get parameters from either GET or POST
if ($request_method === 'POST') {
    $input = $_POST;
} else {
    $input = $_GET;
}

// Check if data is being sent
if (isset($input['node_name']) && (isset($input['temperature']) || isset($input['humidity']))) {
    
    $node_name = $input['node_name'];
    $temperature = isset($input['temperature']) ? floatval($input['temperature']) : null;
    $humidity = isset($input['humidity']) ? floatval($input['humidity']) : null;
    
    // If time not provided, use current timestamp
    if (isset($input['time_received']) && !empty($input['time_received'])) {
        $time_received = $input['time_received'];
    } else {
        $time_received = date('Y-m-d H:i:s');
    }
    
    // Validate data ranges
    $errors = [];
    if ($temperature !== null && ($temperature < -10 || $temperature > 100)) {
        $errors[] = "Temperature must be between -10 and 100°C";
    }
    if ($humidity !== null && ($humidity < 0 || $humidity > 100)) {
        $errors[] = "Humidity must be between 0 and 100%";
    }
    
    if (!empty($errors)) {
        echo json_encode(["status" => "error", "message" => implode(", ", $errors)]);
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
        exit();
    }
    $check_duplicate->close();
    
    // Insert data
    $insert = $conn->prepare("INSERT INTO sensor_data (node_name, time_received, temperature, humidity) VALUES (?, ?, ?, ?)");
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
        echo json_encode(["status" => "error", "message" => "Insert failed: " . $insert->error]);
    }
    $insert->close();
    
} else {
    // No data sent - display current database contents
    ?>
    <!DOCTYPE html>
    <html>
    <head>
        <title>Sensor Data Insert System</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                max-width: 1200px;
                margin: 20px auto;
                padding: 20px;
                background-color: #f5f5f5;
            }
            h1 {
                color: #003366;
            }
            .info-box {
                background-color: #e6f3ff;
                padding: 15px;
                border-radius: 5px;
                margin: 20px 0;
            }
            .security-note {
                background-color: #fff3cd;
                padding: 15px;
                border-left: 4px solid #ffc107;
                margin: 20px 0;
            }
            .example {
                background-color: #f0f0f0;
                padding: 10px;
                border-left: 4px solid #0066cc;
                margin: 10px 0;
                font-family: monospace;
            }
            table {
                width: 100%;
                border-collapse: collapse;
                background-color: white;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
                margin: 20px 0;
            }
            th {
                background-color: #9ACD32;
                color: white;
                padding: 12px;
                text-align: left;
            }
            td {
                padding: 10px;
                border-bottom: 1px solid #ddd;
            }
            tr:nth-child(even) {
                background-color: #f9f9f9;
            }
        </style>
    </head>
    <body>
        <h1>Sensor Data Insert System (Secured with API Key)</h1>
        
        <div class="security-note">
            <h2>🔒 Authentication Required</h2>
            <p>This API requires authentication. Include one of the following headers:</p>
            <ul>
                <li><strong>X-API-Key:</strong> MySecretKey2025_Sensors_98765</li>
                <li><strong>Authorization:</strong> Bearer MySecretKey2025_Sensors_98765</li>
            </ul>
        </div>
        
        <div class="info-box">
            <h2>How to Insert Data</h2>
            <p><strong>Method 1: GET with Headers</strong></p>
            <div class="example">
                curl -H "X-API-Key: MySecretKey2025_Sensors_98765" "<?php echo 'https://' . $_SERVER['HTTP_HOST'] . $_SERVER['PHP_SELF']; ?>?node_name=node_1&temperature=25.5&humidity=60.2"
            </div>
            
            <p><strong>Method 2: POST with Headers (Bearer Token)</strong></p>
            <div class="example">
                curl -X POST "<?php echo 'https://' . $_SERVER['HTTP_HOST'] . $_SERVER['PHP_SELF']; ?>" \<br>
                -H "Authorization: Bearer MySecretKey2025_Sensors_98765" \<br>
                -d "node_name=node_1&temperature=25.5&humidity=60.2"
            </div>
            
            <p><strong>Method 3: Postman</strong></p>
            <ul>
                <li>Go to Headers tab</li>
                <li>Add: X-API-Key = MySecretKey2025_Sensors_98765</li>
                <li>OR Add: Authorization = Bearer MySecretKey2025_Sensors_98765</li>
            </ul>
            
            <h3>Requirements:</h3>
            <ul>
                <li>Valid API Key or Bearer Token required</li>
                <li>Node must be registered in sensor_register table</li>
                <li>Temperature: -10 to 100°C</li>
                <li>Humidity: 0 to 100%</li>
                <li>No duplicate timestamps for same node</li>
                <li>Time is auto-generated if not provided</li>
            </ul>
        </div>
        
        <h2>Current Sensor Data (Sorted by Node Name)</h2>
        <?php
        $query = "SELECT node_name, time_received, temperature, humidity 
                  FROM sensor_data 
                  ORDER BY node_name, time_received";
        $result = $conn->query($query);
        
        if ($result && $result->num_rows > 0) {
            echo '<table>';
            echo '<tr><th>Node Name</th><th>Time Received</th><th>Temperature (°C)</th><th>Humidity (%)</th></tr>';
            while($row = $result->fetch_assoc()) {
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
        ?>
        
        <h2>Registered Nodes</h2>
        <?php
        $nodes_query = "SELECT node_name, manufacturer FROM sensor_register ORDER BY node_name";
        $nodes_result = $conn->query($nodes_query);
        
        if ($nodes_result && $nodes_result->num_rows > 0) {
            echo '<table>';
            echo '<tr><th>Node Name</th><th>Manufacturer</th></tr>';
            while($row = $nodes_result->fetch_assoc()) {
                echo '<tr>';
                echo '<td>' . htmlspecialchars($row['node_name']) . '</td>';
                echo '<td>' . htmlspecialchars($row['manufacturer']) . '</td>';
                echo '</tr>';
            }
            echo '</table>';
        }
        ?>
    </body>
    </html>
    <?php
}

$conn->close();

?>
