// Function to get current readings on the webpage when it loads for the first time
function updateZoneStatus() {
  // $.get("/zoneStatus", (data) => {
  //   var myObj = JSON.parse(this.responseText);
  //   console.log(myObj);
  //   document.getElementById("temp").innerHTML = myObj.temperature;
  //   document.getElementById("hum").innerHTML = myObj.humidity;
  //   document.getElementById("pres").innerHTML = myObj.pressure;
  //   updateDateTime();
  // });
}

function toggleZone(zoneId, status) {
  $.get("/toggleZone?zoneId=" + zoneId + "&status=" + (status ? 1 : 0))
    .done(function (response) {
      console.log("toggleZone POST success", response);
    })
    .fail(function (error) {
      console.error("toggleZone POST error: " + error.statusText);
    });
}

// Create an Event Source to listen for events
if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function (e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function (e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('new_readings', function (e) {
    console.log("new_readings", e.data);
    // var obj = JSON.parse(e.data);
    // document.getElementById("temp").innerHTML = obj.temperature;
    // document.getElementById("hum").innerHTML = obj.humidity;
    // document.getElementById("pres").innerHTML = obj.pressure;
  }, false);
}

$(() => {
  console.log("ready!");
  updateZoneStatus();
});
