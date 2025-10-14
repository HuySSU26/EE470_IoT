<?php
if (function_exists('opcache_reset')) {
    opcache_reset();
    echo "OPcache cleared successfully!<br>";
} else {
    echo "OPcache is not enabled or opcache_reset function not available.<br>";
}

// Also try clearing other caches
if (function_exists('apc_clear_cache')) {
    apc_clear_cache();
    echo "APC cache cleared!<br>";
}

if (function_exists('apcu_clear_cache')) {
    apcu_clear_cache();
    echo "APCu cache cleared!<br>";
}

echo "<br>Cache clearing attempted.";
echo "<br><a href='sensor_dashboard.php'>Go back to dashboard</a>";
?>