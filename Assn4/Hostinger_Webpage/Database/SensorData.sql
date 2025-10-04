-- ========================================
-- DROP EXISTING TABLES (if rerunning)
-- ========================================
 DROP VIEW IF EXISTS sensor_combined;
 DROP TABLE IF EXISTS sensor_data;
 DROP TABLE IF EXISTS sensor_register;

-- ========================================
-- CREATE TABLES
-- ========================================

-- Create sensor_register table first (parent table)
CREATE TABLE sensor_register (
    node_name VARCHAR(10) PRIMARY KEY,
    manufacturer VARCHAR(10) NOT NULL,
    longitude DECIMAL(15, 10) NOT NULL,  -- Supports format like -123.4567890123
    latitude DECIMAL(15, 10) NOT NULL
);

-- Create sensor_data table with foreign key
CREATE TABLE sensor_data (
    data_id INT AUTO_INCREMENT PRIMARY KEY,
    node_name VARCHAR(10) NOT NULL,
    time_received DATETIME NOT NULL,
    temperature DECIMAL(6, 2) NOT NULL,
    humidity DECIMAL(6, 2) NOT NULL,
    FOREIGN KEY (node_name) REFERENCES sensor_register(node_name)
);

-- ========================================
-- INSERT SENSOR REGISTRATIONS (5 sensors)
-- ========================================
INSERT INTO sensor_register (node_name, manufacturer, longitude, latitude)
VALUES
('node_1', 'Bosch', -122.4194, 37.7749),      -- San Francisco
('node_2', 'Honeywell', -118.2437, 34.0522),  -- Los Angeles
('node_3', 'Siemens', -87.6298, 41.8781),     -- Chicago
('node_4', 'Schneider', -95.3698, 29.7604),   -- Houston
('node_5', 'ABB', -75.1652, 39.9526);         -- Philadelphia

-- ========================================
-- INSERT SENSOR DATA (4 readings per sensor, every 30 minutes)
-- Starting: October 1, 2022, 11:00 AM
-- ========================================

-- Node 1 data (4 readings, 30-minute intervals)
INSERT INTO sensor_data (node_name, time_received, temperature, humidity)
VALUES
('node_1', '2022-10-01 11:00:00', 22.5, 45.3),
('node_1', '2022-10-01 11:30:00', 23.1, 46.8),
('node_1', '2022-10-01 12:00:00', 24.2, 47.5),
('node_1', '2022-10-01 12:30:00', 25.0, 48.2);

-- Node 2 data
INSERT INTO sensor_data (node_name, time_received, temperature, humidity)
VALUES
('node_2', '2022-10-01 11:00:00', 28.3, 52.1),
('node_2', '2022-10-01 11:30:00', 29.0, 51.8),
('node_2', '2022-10-01 12:00:00', 29.8, 50.5),
('node_2', '2022-10-01 12:30:00', 30.2, 49.9);

-- Node 3 data
INSERT INTO sensor_data (node_name, time_received, temperature, humidity)
VALUES
('node_3', '2022-10-01 11:00:00', 18.5, 62.3),
('node_3', '2022-10-01 11:30:00', 19.2, 61.5),
('node_3', '2022-10-01 12:00:00', 20.1, 60.8),
('node_3', '2022-10-01 12:30:00', 20.8, 60.0);

-- Node 4 data
INSERT INTO sensor_data (node_name, time_received, temperature, humidity)
VALUES
('node_4', '2022-10-01 11:00:00', 32.1, 78.5),
('node_4', '2022-10-01 11:30:00', 32.8, 77.9),
('node_4', '2022-10-01 12:00:00', 33.5, 76.8),
('node_4', '2022-10-01 12:30:00', 34.0, 75.5);

-- Node 5 data
INSERT INTO sensor_data (node_name, time_received, temperature, humidity)
VALUES
('node_5', '2022-10-01 11:00:00', 21.0, 55.2),
('node_5', '2022-10-01 11:30:00', 21.8, 54.8),
('node_5', '2022-10-01 12:00:00', 22.5, 54.0),
('node_5', '2022-10-01 12:30:00', 23.2, 53.5);

-- ========================================
-- CREATE VIEW (combines both tables)
-- ========================================
CREATE VIEW sensor_combined AS
SELECT 
    sd.data_id,
    sd.node_name,
    sr.manufacturer,
    sr.longitude,
    sr.latitude,
    sd.time_received,
    sd.temperature,
    sd.humidity
FROM sensor_data sd
JOIN sensor_register sr ON sd.node_name = sr.node_name
ORDER BY sd.time_received, sd.node_name;

-- ========================================
-- VERIFICATION QUERIES
-- ========================================

-- View all registered sensors
SELECT * FROM sensor_register;

-- View all sensor data
SELECT * FROM sensor_data ORDER BY time_received, node_name;

-- View the combined data
SELECT * FROM sensor_combined;

-- Count readings per sensor (should be 4 each)
SELECT node_name, COUNT(*) as reading_count
FROM sensor_data
GROUP BY node_name;
