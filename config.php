<?php
    //connect to database
    $server = "localhost";
    $user = "khoa";
    $pass = "123456";
    $dbname = "smart_garden";

    $conn = mysqli_connect($server, $user, $pass, $dbname);

    //check connection
    if($conn == false){
        DIE("ERROR: Could not connect. " . mysqli_connect_error());
    }
?>