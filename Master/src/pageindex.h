#ifndef PAGEINDEX_H
#define PAGEINDEX_H

const char PAGEINDEX[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Slave Data</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #f4f4f4;
      margin: 0;
      padding: 0;
    }

    .container {
      text-align: center;
      padding: 50px;
    }

    .status-container {
      display: flex;
      justify-content: center;
      gap: 20px;
      flex-wrap: wrap;
    }

    .status-box {
      border: 2px solid #4CAF50;
      padding: 20px;
      border-radius: 15px;
      box-shadow: 2px 2px 15px rgba(0, 0, 0, 0.2);
      background-color: #fff;
      font-size: 22px;
      font-weight: bold;
      color: #333;
      width: 300px;
      text-align: center;
    }

    .status-box h2 {
      font-size: 24px;
      color: #4CAF50;
      margin-bottom: 10px;
    }

    .status-box p {
      font-size: 18px;
      margin: 5px 0;
    }

    .status-box .cooldown {
      font-size: 16px;
      color: #FF6347;
      font-style: italic;
    }
  </style>
  <script>
    // Function to fetch the latest data
  function fetchData() {
  fetch("/status")
    .then(response => response.text())
    .then(data => {
      const statusData = data.split(', ');

      // Extract data for all slaves
      const slave1Status = statusData[0].split(": ")[1];
      const slave2Temp = statusData[1].split(": ")[1];
      const slave2FanStatus = statusData[2].split(": ")[1];
      const slave3WaterLevel = statusData[3].split(": ")[1];
      const slave3RefillStatus = statusData[4].split(": ")[1];
      const slave3SoilStatus = statusData[5].split(": ")[1];
      const slave3PumpStatus = statusData[6].split(": ")[1]; // This will contain "Watering" or "Waiting for cooldown"
      const remainingCooldown = statusData.length > 7 ? statusData[7].split(": ")[1] : "";

      // Update the content dynamically
      document.getElementById("slave1Status").innerText = "Light Status: " + slave1Status;
      document.getElementById("slave2Temp").innerText = "Temperature: " + slave2Temp + " °C";
      document.getElementById("slave2FanStatus").innerText = "Fan Status: " + slave2FanStatus;
      document.getElementById("slave3WaterLevel").innerText = "Water Level: " + slave3WaterLevel;
      document.getElementById("slave3RefillStatus").innerText = "Water Container: " + slave3RefillStatus;

      // Handle soil status and pump status with cooldown logic
      if (slave3SoilStatus === "Dry") {
        if (remainingCooldown) {
          document.getElementById("slave3SoilStatus").innerText = "Soil Status: Dry (Waiting for cooldown)";
          document.getElementById("slave3PumpStatus").innerText = "Water for plant: Waiting for cooldown";
        } else {
          document.getElementById("slave3SoilStatus").innerText = "Soil Status: Dry (Watering)";
          document.getElementById("slave3PumpStatus").innerText = "Water for plant: Watering";
        }
      } else {
        document.getElementById("slave3SoilStatus").innerText = "Soil Status: " + slave3SoilStatus;
        document.getElementById("slave3PumpStatus").innerText = "Water for plant: " + slave3PumpStatus;
      }

      // Show remaining cooldown message if it exists
      if (remainingCooldown) {
        document.getElementById("cooldownMessage").innerText = "Remaining cooldown: " + remainingCooldown + " seconds";
        document.getElementById("cooldownMessage").style.display = "block";
      } else {
        document.getElementById("cooldownMessage").style.display = "none";  // Hide if no cooldown
      }
    })
    .catch(error => console.error('Error fetching data:', error));
}

    // Call fetchData every 2 seconds
    setInterval(fetchData, 2000);
  </script>
</head>
<body>
  <div class="container">
    <h1>Green Green Grass of Home</h1>
    <div class="status-container">
      <div class="status-box">
        <h2>ESP-A</h2>
        <h2>LDR Sensor</h2>
        <p id="slave1Status">Light Status: Loading...</p>
      </div>
      <div class="status-box">
        <h2>ESP-B</h2>
        <h2>DHT11</h2>
        <p id="slave2Temp">Temperature: Loading...</p>
        <p id="slave2FanStatus">Fan Status: Loading...</p>
      </div>
      <div class="status-box">
        <h2>ESP-C.1</h2>
        <h2>Water Sensor</h2>
        <p id="slave3WaterLevel">Water Level: Loading...</p>
        <p id="slave3RefillStatus">Water Container: Loading...</p>
      </div>

      <div class="status-box">
        <h2>ESP-C.2</h2>
        <h2>Moisture Sensor</h2>
        <p id="slave3SoilStatus">Soil Status: Loading...</p>
        <p id="slave3PumpStatus">Water for plant: %WATER_FOR_PLANT%</p>
        <p id="cooldownMessage" class="cooldown" style="display:none;"></p> <!-- Cooldown message -->
      </div>
    </div>
  </div>
</body>
</html>
)rawliteral";

#endif
