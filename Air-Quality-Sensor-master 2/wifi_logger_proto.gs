/* 

COMMUNITY SENSOR LAB, ASRC, CUNY, NYC
Arduino-based sensor nodes POSTing through WiFi to Google Sheets 
Amalia Torres, Ricardo Toledo-Crow, March 2021

HTML POST to Google Sheets using REST API.

1- Copy the Google Sheet to your Google space or make a new one. Give it a name.
2- Open scripts editor: Tools->Script Editor
3- Copy and paste this entire script code into the script editor
4- Name the script like the Google Sheet
5- From the URL of the Google Sheet, copy the alphanumeric GoogleSheet_ID; it's between "d/" and  "/edit"
6- paste the ID in line 55 below replacing 'GoogleSheet_ID'
7- Save this script
8- Goto Deploy button in upper right (new editor. If using old editor its: Publish->Deploy as web app...)
9- Enter Description: e.g. test deployment no 1
10- Execute app as: Me.
11- Who has access: Anyone
12- Make sure its a Web app and hit Deploy
13- Copy the Deployment_ID (the long alphanumeric string) and paste it into the Arduino Code as shown below in line 26

Arduino code to do the actual posting to this script: https://github.com/Community-Sensor-Lab/Air-Quality-Sensor

This code Posts the following: 
  client.println("POST /macros/s/Deployment_ID/exec?value=Hello HTTP/1.1" // the Deployment_ID  
  client.println("Host: script.google.com");
  client.println("Content-Type: application/x-www-form-urlencoded");
  //client.println("Connection: close");
  client.print("Content-Length: ");
  client.println(payload.length());
  client.println();
  client.print(payload);
  client.println();

where:
  sprintf(outstr, "%02u/%02u/%02u %02u:%02u:%02u, %.2d, %.2f, %.2f, %.2f, %.2f, %.2f, %.2f, %x",year,month,day,hour,minute,second,CO2,Tco2,RHco2,Tbme,Pbme,RHbme,measuredvbat,stat);
  String payload = "{\"command\":\"appendRow\",\"sheet_name\":\"Sheet1\",\"values\":" + "\"" + outstr + "\"}"

and:
CO2: co2 value from Sensirion SCD30 sensor in ppm
Tco2: temperature from Sensirion SCD30 sensor in degs C
RHco2: relative humidity from Sensirion SCD30 sensor in %
Tbme: temperature from BME280 sensor in degs C
Pbme: pressure from BME280 sensor in hPa or mTorr
RHbme: relative humidity from BME280 sensor in %
measuredvbat: voltage of LiPoly battery (3.7V nominal)
stat: staus of sensor node

This script based on Github: https://github.com/electronicsguy

*/

// The doPost() needs the spreadsheet ID and not the "active spreadsheet" since the device can operate without the spreadsheet being open.
var SS = SpreadsheetApp.openById('GoogleSheet_ID'); // ID is alphanumeric string in the Google Sheet's URL

function doPost(e) {

  var parsedData;
  
  try { 
    parsedData = JSON.parse(e.postData.contents);
  } 
  catch(f){
    return ContentService.createTextOutput("Error in parsing request body: " + f.message);
  }
   
  if (parsedData !== undefined) {

    switch (parsedData.command) {
      case "appendRow":     // this command must come in the JSON object
        var tmp = SS.getSheetByName(parsedData.sheet_name);
        var dataArr = parsedData.values.split(",");   // get the values and split them into an array
        var d = new Date(); // get current datetime
        dformat = [d.getFullYear(), d.getMonth() + 1, d.getDate()].join('/') + ' ' + // convert to simple format yyyy/mm/dd hh:mm:ss
          [d.getHours(), d.getMinutes(), d.getSeconds()].join(':');

        dataArr.unshift(dformat); // add the system datetime to beginning of array
        tmp.appendRow(dataArr); // add row to gsheet 

        str = "Success";
        SpreadsheetApp.flush();
        break;
    }
    return ContentService.createTextOutput(str);
  } // endif (parsedData !== undefined)

  else {
    return ContentService.createTextOutput("Error! Request body empty or in incorrect format.");
  }
}
