<?php
/**
 * RGB Value Proxy - Universal (HTTP/HTTPS)
 * =========================================
 * Works with both HTTP and HTTPS
 * Returns plain text RGB values
 */

// Force plain text output (no HTML)
header('Content-Type: text/plain; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Cache-Control: no-cache, no-store, must-revalidate');
header('Pragma: no-cache');
header('Expires: 0');

// Disable any buffering
if (ob_get_level()) ob_end_clean();

// Path to RGB values file
$file = __DIR__ . '/rgb_value.txt';

// Read and output - NOTHING ELSE
if (file_exists($file)) {
    $content = @file_get_contents($file);
    if ($content !== false) {
        echo trim($content);
    } else {
        echo "0,0,0";
    }
} else {
    echo "0,0,0";
}

// Force output and stop
exit;
?>