<?php
/**
 * LED Control API - Enhanced Version
 * Handles GET and PUT requests to control ESP8266 LEDs
 * Writes to result.txt for microcontroller to read
 * 
 * Endpoints:
 * - GET: Returns current LED states
 * - PUT/POST: Updates LED states
 */

//////////////////// Headers (CORS + no-cache) ////////////////////
header('Content-Type: application/json; charset=utf-8');
header('Access-Control-Allow-Origin: *');
header('Access-Control-Allow-Methods: GET, POST, PUT, OPTIONS');
header('Access-Control-Allow-Headers: Content-Type, X-Requested-With');
header('Cache-Control: no-store, no-cache, must-revalidate, max-age=0');
header('Pragma: no-cache');
header('Expires: 0');

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
  http_response_code(204);
  exit;
}

//////////////////// Settings ////////////////////
date_default_timezone_set('America/Los_Angeles');
$logFile       = __DIR__ . '/result.txt'; // history file (JSON Lines)
$maxLogEntries = 2000;                    // keep last N entries in file (set null to disable)
$defaultLimit  = 50;                      // default history items in GET response
$maxLimit      = 1000;                    // hard cap for GET limit

//////////////////// Helpers ////////////////////
function now_local(): string {
  return date('c'); // ISO-like with timezone offset
}

function normalize_onoff($v): ?string {
  if (!isset($v)) return null;
  $v = strtoupper(trim((string)$v));
  return ($v === 'ON' || $v === 'OFF') ? $v : null;
}

/** Parse input as JSON or form-encoded */
function parse_input(): array {
  $method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
  $ctype  = $_SERVER['CONTENT_TYPE'] ?? $_SERVER['HTTP_CONTENT_TYPE'] ?? '';

  if ($method === 'PUT' || stripos($ctype, 'application/json') !== false) {
    $raw = file_get_contents('php://input');
    if ($raw !== false && $raw !== '') {
      $j = json_decode($raw, true);
      if (is_array($j)) return $j;
    }
  }
  return $_POST ?: [];
}

/** Read the latest state (last non-empty valid JSON line) */
function read_latest_state(string $file): array {
  if (!is_file($file) || filesize($file) === 0) {
    return ['led1' => 'OFF', 'led2' => 'OFF', 'timestamp' => now_local()];
  }

  $fh = fopen($file, 'r');
  if (!$fh) return ['led1' => 'OFF', 'led2' => 'OFF', 'timestamp' => now_local()];
  @flock($fh, LOCK_SH);

  $size    = fstat($fh)['size'] ?? 0;
  $buffer  = '';
  $pointer = max(0, $size - 1);

  while ($pointer >= 0) {
    fseek($fh, $pointer);
    $ch = fgetc($fh);
    if ($ch === "\n") {
      if ($buffer !== '') break;
    } else {
      $buffer = $ch . $buffer;
    }
    $pointer--;
  }

  @flock($fh, LOCK_UN);
  fclose($fh);

  $line = trim($buffer);
  $latest = json_decode($line, true);
  if (is_array($latest)) {
    $latest += ['led1' => 'OFF', 'led2' => 'OFF', 'timestamp' => now_local()];
    return $latest;
  }
  return ['led1' => 'OFF', 'led2' => 'OFF', 'timestamp' => now_local()];
}

/**
 * Tail last N JSON lines efficiently (newest first).
 * Returns an array of decoded objects. If $skipFirst is true, skips the very last line (the "latest").
 */
function tail_jsonl(string $file, int $n, bool $skipFirst = false): array {
  $out = [];
  if (!is_file($file) || $n <= 0) return $out;

  $fh = fopen($file, 'r');
  if (!$fh) return $out;
  @flock($fh, LOCK_SH);

  $size    = fstat($fh)['size'] ?? 0;
  if ($size === 0) { @flock($fh, LOCK_UN); fclose($fh); return $out; }

  $lines   = 0;
  $buffer  = '';
  $pointer = $size - 1;

  // First, optionally skip the very last line (for "history" separate from latest)
  $skipped = false;

  while ($pointer >= 0 && $lines < ($n + ($skipFirst ? 1 : 0))) {
    fseek($fh, $pointer);
    $ch = fgetc($fh);

    if ($ch === "\n") {
      if ($buffer !== '') {
        if ($skipFirst && !$skipped) {
          // drop the most recent full line
          $buffer = '';
          $skipped = true;
        } else {
          $obj = json_decode($buffer, true);
          if (is_array($obj)) $out[] = $obj;
          $lines++;
          $buffer = '';
        }
      }
    } else {
      $buffer = $ch . $buffer;
    }
    $pointer--;
  }

  // Capture the first line if we hit BOF without newline
  if ($buffer !== '' && $lines < ($n + ($skipFirst ? 1 : 0))) {
    if (!($skipFirst && !$skipped)) {
      $obj = json_decode($buffer, true);
      if (is_array($obj)) $out[] = $obj;
    }
  }

  @flock($fh, LOCK_UN);
  fclose($fh);

  return $out; // newest-first order due to reverse scan
}

/** Append new state as JSON line and optionally trim file */
function append_state(string $file, array $state, ?int $maxEntries = 2000): void {
  $fh = fopen($file, 'a');
  if ($fh) {
    @flock($fh, LOCK_EX);
    fwrite($fh, json_encode($state, JSON_UNESCAPED_SLASHES) . "\n");
    fflush($fh);
    @flock($fh, LOCK_UN);
    fclose($fh);
  }

  if ($maxEntries !== null && $maxEntries > 0) {
    $lines = @file($file, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
    if ($lines && count($lines) > $maxEntries) {
      $slice = array_slice($lines, -$maxEntries);
      $tmp = $file . '.tmp';
      @file_put_contents($tmp, implode("\n", $slice) . "\n");
      @rename($tmp, $file);
      @chmod($file, 0644);
    }
  }
}

//////////////////// Main ////////////////////
$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';

if ($method === 'GET') {
  $limit = isset($_GET['limit']) ? (int)$_GET['limit'] : $defaultLimit;
  if ($limit < 0) $limit = 0;
  if ($limit > $maxLimit) $limit = $maxLimit;

  $latest  = read_latest_state($logFile);
  $history = $limit > 0 ? tail_jsonl($logFile, $limit, /*skipFirst*/ true) : [];

  echo json_encode(
    ['status' => 'success'] + $latest + ['history' => $history],
    JSON_UNESCAPED_SLASHES
  );
  exit;
}

if ($method === 'POST' || $method === 'PUT') {
  $incoming = parse_input();
  if (empty($incoming)) {
    http_response_code(400);
    echo json_encode(['status' => 'error', 'message' => 'Empty or invalid payload']);
    exit;
  }

  $state = read_latest_state($logFile);

  $n1 = normalize_onoff($incoming['led1'] ?? null);
  $n2 = normalize_onoff($incoming['led2'] ?? null);
  if ($n1 !== null) $state['led1'] = $n1;
  if ($n2 !== null) $state['led2'] = $n2;

  // Always bump timestamp so clients can detect a fresh update
  $state['timestamp'] = now_local();

  append_state($logFile, $state, $maxLogEntries);

  echo json_encode(['status' => 'success'] + $state, JSON_UNESCAPED_SLASHES);
  exit;
}

http_response_code(405);
echo json_encode(['status' => 'error', 'message' => 'Method Not Allowed']);
?>