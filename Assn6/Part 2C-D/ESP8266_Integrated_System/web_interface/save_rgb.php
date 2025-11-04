<?php
/**
 * Save RGB Values
 * Handles POST requests to save RGB values to rgb_value.txt
 */

header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: POST, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $rgb = $_POST['rgb'] ?? '';
    
    if (empty($rgb)) {
        http_response_code(400);
        echo json_encode([
            'status' => 'error',
            'message' => 'No RGB data provided'
        ]);
        exit;
    }
    
    // Validate format (should be R,G,B)
    $values = explode(',', $rgb);
    if (count($values) !== 3) {
        http_response_code(400);
        echo json_encode([
            'status' => 'error',
            'message' => 'Invalid RGB format. Expected: R,G,B'
        ]);
        exit;
    }
    
    // Validate each value is 0-255
    foreach ($values as $value) {
        $val = intval(trim($value));
        if ($val < 0 || $val > 255) {
            http_response_code(400);
            echo json_encode([
                'status' => 'error',
                'message' => 'RGB values must be between 0 and 255'
            ]);
            exit;
        }
    }
    
    // Save to file
    $file = __DIR__ . '/rgb_value.txt';
    $result = file_put_contents($file, $rgb);
    
    if ($result !== false) {
        echo json_encode([
            'status' => 'success',
            'rgb' => $rgb,
            'timestamp' => date('Y-m-d H:i:s')
        ]);
    } else {
        http_response_code(500);
        echo json_encode([
            'status' => 'error',
            'message' => 'Failed to save RGB values'
        ]);
    }
    exit;
}

http_response_code(405);
echo json_encode([
    'status' => 'error',
    'message' => 'Method not allowed'
]);
?>
