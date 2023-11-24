<?php
$servername = "localhost";
$username = "XXX"; // Your database username
$password = "XXX"; // Your database password
$dbname = "GreenhouseDB";        //<<<<<<<<<<<<<<<
$tableName = "DB_4";             //<<<<<<<<<<<<<<<


// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// SQL query to fetch data
$sql = "SELECT timestamp, temperature, humidity FROM `$tableName` ORDER BY time>
$result = $conn->query($sql);

// Arrays to store data
$timestamp = array();
$temperatures = array();
$humidities = array();

// Fetch data
if ($result->num_rows > 0) {
    while($row = $result->fetch_assoc()) {
        array_push($temperatures, $row["temperature"]);
        array_push($humidities, $row["humidity"]);
        array_push($timestamp, $row["timestamp"]);
}
} else {
    echo "0 results";
}

$conn->close();
?>

<!DOCTYPE html>
<html>
<head>
    <title>GreenHouse Monitoring</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>GreenHouse Monitoring <?php echo json_encode($tableName); ?> <h1>
<div style="width:75%;">
    <h2>Temperature</h2>
    <canvas id="temperatureChart"></canvas>
    <h2>Humidity</h2>
    <canvas id="humidityChart"></canvas>

</div>

<script>
    var ctxTemp = document.getElementById('temperatureChart').getContext('2d');
    var temperatureChart = new Chart(ctxTemp, {
        type: 'line',
        data: {
            labels: <?php echo json_encode(array_reverse($timestamp)); ?>,
            datasets: [{
                label: 'Temperature',
                data: <?php echo json_encode(array_reverse($temperatures)); ?>,
                borderColor: 'red',
                fill: false
            }]
        },
        options: {
            scales: {
                y: {
                    beginAtZero: false
                }
            }
        }
    });

    var ctxHumidity = document.getElementById('humidityChart').getContext('2d');
    var humidityChart = new Chart(ctxHumidity, {
        type: 'line',
        data: {
            labels: <?php echo json_encode(array_reverse($timestamp)); ?>,
            datasets: [{
                label: 'Humidity',
                data: <?php echo json_encode(array_reverse($humidities)); ?>,
                borderColor: 'blue',
                fill: false
            }]
        },
        options: {
            scales: {
                y: {
                    beginAtZero: false
                }
            }
        }
    });


</script>

</body>
</html>
