<?php
$servername = "localhost";
$username = "XXX"; // Your MySQL username
$password = "XXX"; // Your MySQL password
$dbname = "GreenhouseDB";      //<<<<<<<<<<<<<<<<
$tableName = "DB_4";           //<<<<<<<<<<<<<<<<

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

$temperature = $_GET['temperature'];
$humidity = $_GET['humidity'];

// Check if the table exists, and if not, create it
$table_check = "SHOW TABLES LIKE '$tableName'";
$result = $conn->query($table_check);
if (!$result) {
    die("Query failed: " . $conn->error);
}

if ($result->num_rows == 0) {
    $create_table = "CREATE TABLE `$tableName` (
        id INT AUTO_INCREMENT PRIMARY KEY,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        temperature FLOAT,
        humidity FLOAT
    )";

    if ($conn->query($create_table) === TRUE) {
        echo "Table $tableName created successfully";
    } else {
        echo "Error creating table: " . $conn->error;
    }
}

$sql = "INSERT INTO `$tableName` (temperature, humidity) VALUES ('$temperature'>

if ($conn->query($sql) === TRUE) {
    echo "New record created successfully";
} else {
    echo "Error: " . $sql . "<br>" . $conn->error;
}

$conn->close();
?>
