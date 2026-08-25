/*
  PneumoGuard - Google Apps Script Web App (caregiver dashboard backend)
  ---------------------------------------------------------------------------
  SETUP:
    1. sheets.google.com -> new spreadsheet -> rename "PneumoGuard Dashboard"
    2. Extensions -> Apps Script -> delete placeholder -> paste this whole file
    3. Deploy -> New deployment -> Web app
       - Execute as: Me
       - Who has access: Anyone
    4. Deploy, authorize it, copy the Web app URL (ends in /exec)
    5. Paste that URL into DASHBOARD_URL in firmware/pneumoguard_firmware.ino
       (for POST) AND into the dashboard webpage's SCRIPT_URL constant (for GET)

  doPost -> called by the ESP32 to log a new reading (appends a row)
  doGet  -> called by the dashboard webpage to read back recent readings

  Field names below match firmware/pneumoguard_firmware.ino's logToCloud()
  JSON payload exactly: pressure_hPa, temperature_C, accel_variance,
  pressure_drop, context, risk.
*/

function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

  if (sheet.getLastRow() === 0) {
    sheet.appendRow([
      "Timestamp (server)", "Pressure (hPa)", "Temperature (C)",
      "Accel Variance", "Pressure Drop (hPa)", "Context", "Risk Level"
    ]);
  }

  var data = JSON.parse(e.postData.contents);

  sheet.appendRow([
    new Date(),
    data.pressure_hPa,
    data.temperature_C,
    data.accel_variance,
    data.pressure_drop,
    data.context,
    data.risk
  ]);

  return ContentService
    .createTextOutput(JSON.stringify({ status: "ok" }))
    .setMimeType(ContentService.MimeType.JSON);
}

function doGet(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var lastRow = sheet.getLastRow();

  if (lastRow < 2) {
    return ContentService
      .createTextOutput(JSON.stringify({ readings: [] }))
      .setMimeType(ContentService.MimeType.JSON);
  }

  // Return the most recent 50 readings (or fewer if there aren't that many yet)
  var maxRows = 50;
  var startRow = Math.max(2, lastRow - maxRows + 1);
  var numRows = lastRow - startRow + 1;

  var values = sheet.getRange(startRow, 1, numRows, 7).getValues();

  var readings = values.map(function(row) {
    return {
      timestamp: row[0] instanceof Date ? row[0].toISOString() : row[0],
      pressure_hPa: row[1],
      temperature_C: row[2],
      accel_variance: row[3],
      pressure_drop: row[4],
      context: row[5],
      risk: row[6]
    };
  });

  return ContentService
    .createTextOutput(JSON.stringify({ readings: readings }))
    .setMimeType(ContentService.MimeType.JSON);
}