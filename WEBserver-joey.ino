#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "DHT.h"
#include "ArduinoJson.h"
#include "Adafruit_Sensor.h"
#include "FS.h"
#include "HTTPClient.h"
#include "BLEDevice.h"
#include "string"

const char* ssid = "saeiomjoey";
const char* password = "joeyjoey1";

// Définition du port de données et du capteur
DHT dht(25, DHT11);

// Création de l'objet serveur web
AsyncWebServer server(80);



// The remote service we wish to connect to.
static BLEUUID serviceUUID("a854f4b4-8af9-4337-880d-745383350edc");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("e7a17637-0e41-4e41-a479-a333c824fc98");
const char* srvble="joeyblesae";

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.getName()==srvble) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks






void setup() {
  // Port série pour les fins de débogage
  Serial.begin(115200);

  // Initialisation de SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Une erreur s'est produite lors du montage de SPIFFS");
    return;
  }

  // Connexion au réseau Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connexion au réseau Wi-Fi en cours...");
  }

  // Affichage de l'adresse IP locale de l'ESP32
  Serial.println(WiFi.localIP());

  // Définition de la route pour la page d'accueil "/"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", String(), false);
  });

    server.on("/data.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/data.json", String(), false);
  });

     server.on("/dataS.json", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/dataS.json", String(), false);
  });

  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  server.on("/datable.json", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/datable.json");
  });     
  server.begin();

  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  // End of setup.
}


void write_to_file(const char* value){
  File data_json=SPIFFS.open("/datable.json", "w");
  data_json.print(value);
  data_json.close();
  
  }


void loop() {
  // Lecture des valeurs du capteur
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // Création de l'objet JSON
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;

  // Convertir l'objet JSON en chaîne
  String jsonStr;
  serializeJson(doc, jsonStr);

  // Afficher la chaîne JSON dans la console série
  Serial.println(jsonStr);

  // Ouverture du fichier JSON en écriture
  File file = SPIFFS.open("/data.json", "w");
  if (!file) {
    Serial.println("Erreur lors de l'ouverture du fichier JSON");
    return;
  }

  // Écrire la chaîne JSON dans le fichier
  file.println(jsonStr);

  // Fermer le fichier
  file.close();

    // Effectuer la requête curl et stocker le résultat dans le fichier dataS.json
  HTTPClient http;
  http.begin("https://api.sigfox.com/v2/devices/B448DD/messages");
  http.setAuthorization("6334446bbdcbd20e4daf92b4", "3806e9d6cead4f45803df27eee00994a");
  int httpResponseCode = http.GET();

  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();

    // Ouverture du fichier JSON en écriture
    File file = SPIFFS.open("/dataS.json", "w");
    if (!file) {
      Serial.println("Erreur lors de l'ouverture du fichier JSON");
      return;
    }

    // Écrire la réponse dans le fichier
    file.println(payload);

    // Fermer le fichier
    file.close();

    Serial.println("Résultat de la requête curl stocké dans le fichier dataS.json");
  } else {
    Serial.print("Erreur lors de la requête curl. Code d'erreur : ");
    Serial.println(httpResponseCode);
  }

  http.end();

  delay(2000);
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  if (connected) {
    if (pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value is: ");
      Serial.println(value.c_str());
      write_to_file(value.c_str());
    }
  }
  
  delay(3000); 
}
