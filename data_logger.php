<?php
$servername = "localhost";
$username = "admin"; // Your MySQL username
$password = "pi"; // Your MySQL password
$dbname = "GreenhouseDB";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$temperature = $_GET['temperature'];
$humidity = $_GET['humidity'];

$sql = "INSERT INTO SensorData (temperature, humidity) VALUES ('$temperature', '$humidity')";

if ($conn->query($sql) === TRUE) {
    echo "New record created successfully";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>
