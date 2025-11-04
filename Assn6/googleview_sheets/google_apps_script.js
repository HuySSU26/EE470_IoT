/**
 * Google Apps Script for Importing Sensor Data into Google Sheets
 * 
 * Instructions:
 * 1. Open your Google Sheet
 * 2. Go to Extensions > Apps Script
 * 3. Delete any existing code and paste this script
 * 4. Update the API_URL to point to your PHP endpoint
 * 5. Save and run the fetchSensorData function
 * 6. You can also set up a time-based trigger for automatic updates
 */

// UPDATE THIS URL to point to your PHP endpoint that returns JSON data
const API_URL = 'https://huynguyen.co/get_sensor_data.php';

/**
 * Fetch sensor data from your PHP endpoint and populate the sheet
 */
function fetchSensorData() {
  try {
    // Fetch data from your PHP endpoint
    const response = UrlFetchApp.fetch(API_URL);
    const jsonData = JSON.parse(response.getContentText());
    
    // Get the active sheet
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    
    // Clear existing data (except header)
    sheet.clear();
    
    // Set headers
    sheet.getRange(1, 1).setValue('Date');
    sheet.getRange(1, 2).setValue('Sensor Value');
    
    // Make headers bold
    sheet.getRange(1, 1, 1, 2).setFontWeight('bold');
    
    // Populate data
    if (jsonData && jsonData.length > 0) {
      const dataArray = jsonData.map(item => [
        item.timestamp,
        parseFloat(item.sensor_value)
      ]);
      
      // Write data starting from row 2
      sheet.getRange(2, 1, dataArray.length, 2).setValues(dataArray);
      
      // Auto-resize columns
      sheet.autoResizeColumns(1, 2);
      
      // Add timestamp of last update
      sheet.getRange(dataArray.length + 3, 1).setValue('Last Updated:');
      sheet.getRange(dataArray.length + 3, 2).setValue(new Date());
      
      Logger.log('Data imported successfully! ' + dataArray.length + ' rows added.');
      SpreadsheetApp.getUi().alert('Success!', 'Data imported successfully! ' + dataArray.length + ' rows added.', SpreadsheetApp.getUi().ButtonSet.OK);
    } else {
      SpreadsheetApp.getUi().alert('No Data', 'No data was returned from the API.', SpreadsheetApp.getUi().ButtonSet.OK);
    }
    
  } catch (error) {
    Logger.log('Error: ' + error.toString());
    SpreadsheetApp.getUi().alert('Error', 'Failed to fetch data: ' + error.toString(), SpreadsheetApp.getUi().ButtonSet.OK);
  }
}

/**
 * Create a custom menu in the spreadsheet
 */
function onOpen() {
  const ui = SpreadsheetApp.getUi();
  ui.createMenu('Sensor Data')
      .addItem('Refresh Data', 'fetchSensorData')
      .addItem('Setup Auto-Update', 'setupTrigger')
      .addItem('Remove Auto-Update', 'removeTrigger')
      .addToUi();
}

/**
 * Setup automatic data refresh (runs every hour)
 */
function setupTrigger() {
  // Delete existing triggers
  removeTrigger();
  
  // Create new hourly trigger
  ScriptApp.newTrigger('fetchSensorData')
      .timeBased()
      .everyHours(1)
      .create();
  
  SpreadsheetApp.getUi().alert('Success', 'Auto-update enabled! Data will refresh every hour.', SpreadsheetApp.getUi().ButtonSet.OK);
}

/**
 * Remove automatic refresh trigger
 */
function removeTrigger() {
  const triggers = ScriptApp.getProjectTriggers();
  for (let i = 0; i < triggers.length; i++) {
    if (triggers[i].getHandlerFunction() === 'fetchSensorData') {
      ScriptApp.deleteTrigger(triggers[i]);
    }
  }
}

/**
 * Create a simple chart visualization
 */
function createChart() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const lastRow = sheet.getLastRow();
  
  if (lastRow < 2) {
    SpreadsheetApp.getUi().alert('Error', 'No data available to create chart.', SpreadsheetApp.getUi().ButtonSet.OK);
    return;
  }
  
  // Remove existing charts
  const charts = sheet.getCharts();
  charts.forEach(chart => sheet.removeChart(chart));
  
  // Create new chart
  const chart = sheet.newChart()
      .setChartType(Charts.ChartType.COLUMN)
      .addRange(sheet.getRange(1, 1, lastRow, 2))
      .setPosition(lastRow + 5, 1, 0, 0)
      .setOption('title', 'Sensor vs Date')
      .setOption('width', 800)
      .setOption('height', 400)
      .setOption('legend', {position: 'bottom'})
      .build();
  
  sheet.insertChart(chart);
  SpreadsheetApp.getUi().alert('Success', 'Chart created successfully!', SpreadsheetApp.getUi().ButtonSet.OK);
}

/**
 * Calculate and display statistics
 */
function calculateStats() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  const lastRow = sheet.getLastRow();
  
  if (lastRow < 2) {
    SpreadsheetApp.getUi().alert('Error', 'No data available to calculate statistics.', SpreadsheetApp.getUi().ButtonSet.OK);
    return;
  }
  
  const dataRange = sheet.getRange(2, 2, lastRow - 1, 1);
  
  // Create stats in columns D and E
  sheet.getRange(2, 4).setValue('MIN:');
  sheet.getRange(2, 5).setFormula('=MIN(B2:B' + lastRow + ')');
  
  sheet.getRange(3, 4).setValue('MAX:');
  sheet.getRange(3, 5).setFormula('=MAX(B2:B' + lastRow + ')');
  
  sheet.getRange(4, 4).setValue('AVG:');
  sheet.getRange(4, 5).setFormula('=AVERAGE(B2:B' + lastRow + ')');
  
  // Format stats
  sheet.getRange(2, 4, 3, 1).setFontWeight('bold');
  sheet.getRange(2, 5, 3, 1).setNumberFormat('0.00');
  
  SpreadsheetApp.getUi().alert('Success', 'Statistics calculated successfully!', SpreadsheetApp.getUi().ButtonSet.OK);
}