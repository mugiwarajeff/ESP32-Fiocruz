//projeto IoT LabVantage iniciado 06/01/2023

//setando configuraçoes do XML Parser
#include <tinyxml2.h>

//lib para tratamento de string
#include <string.h>

using namespace tinyxml2;
char* testDocument = "<root><value>26.74</value></root>";

//setando configuraçoes do BLE

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

//definindo UUID dos serviços e caracteristicas 
#define SERVICE_UUID "ab0828b1-198e-4351-b779-901fa0e0371e"
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"

//Nome do dispositivo
#define bleServerName "IoT_Sensor_phmetro"

//Definindo RS232 Serial
#define RXD2 16
#define TXD2 17


//comando para balança 
#define COMANDO                   "D05\r"

//iniciado aqui pra ter estado global
BLECharacteristic *characteristicTX;

//variavaies de controle
bool deviceConnected = false;
std::string valorRes = "";
std::string rxValue = "";
int w = 0;

const char* convertStringToChar(std::string text){
  int textSize = text.length();
  const char* charReturn =  text.c_str();
  //text.toCharArray(charReturn, textSize);
  
  Serial.println(charReturn);
  
  Serial.println("convertStringToChar");
  Serial.println(charReturn);
  
  return charReturn;
  //return "<root><value>26.74</value></root>";
}

void clearSerial(){
  while(Serial.available() > 0){
    char nextChar = Serial.read();
  }
}

//funçao para se obter valor com base num arquivo XML
double getValueFromXML(const char* xml){

  XMLDocument xmlDocument;
  
  Serial.println("getValueFromXML");
  Serial.println(xml);

  //refatorar o seguinte codigo:
  if(xmlDocument.Parse(xml) != XML_SUCCESS){
    Serial.println("parsing error...");
    return 00.00;
  }

  XMLNode * root = xmlDocument.FirstChild();
  XMLElement * element =  root->FirstChildElement("value");

  double value;
  element->QueryDoubleText(&value);

  return value;

}

double getValueFromCSV(){
  
}

std::string getValueFromSerial(){
  Serial.write(COMANDO);

  std::string fromSerial = "";

  while(fromSerial == ""){
    if(Serial.available()){
      fromSerial = Serial.readString().c_str();
    }
    delay(500);
  }
  
  return fromSerial;
}

//setup callbacks de eventos do servidor
class bleServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
    Serial.println("Dispositivo foi conectado com sucesso");
  }

  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
    Serial.println("Dispositivo foi desconectado com sucesso");

    delay(1000);
    ESP.restart();
  }
};

//criaçao de callbacks para tratamento de envio e recebimento de dados das caracteristicas
class CharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic){
    rxValue = characteristic->getValue();
    
    if(rxValue.length() > 0){
      for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
      }
    Serial.println();

      if(rxValue == "L"){ //se tiver recebido o comando
        std::string serialRes = getValueFromSerial();

        const char* serialReslChar = convertStringToChar(serialRes);
        
        double value = getValueFromXML(serialReslChar);
        Serial.println(value);

        //converte o numero recebido para a string ou array de char de resultado
        char buffer[20];
        std::sprintf(buffer, "%.2f", value); //valorRes = std::to_string(value);
        valorRes = buffer;
      }
    }
  }
};

//setup BLE
void setupBLE(){
  //inicia o dispositivo ble
  BLEDevice::init(bleServerName);

  //cria o servidor ble
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new bleServerCallbacks());

  //cria serviço para leitura de dados da balança
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //cria caracteristica para envio de dados
  characteristicTX = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);

  //cria caracteristica para recebimento de dados
  BLECharacteristic *characteristicRX = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);

  //seta descriçao do ble
  characteristicTX->addDescriptor(new BLE2902());

  //definir o callbacks na caracteristica de recebimento de dados
  characteristicRX->setCallbacks(new CharacteristicCallbacks());

  //iniciar o serviço
  pService->start();

  //permitir descoberta do dispositivo;
  pServer->getAdvertising()->start();
}

void setup() {
  Serial.begin(115200); //Serial para debugging
  //Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); para comunicaçao com equipamento
  setupBLE();
}

void loop() {

  //Serial.write(COMANDO);
  //sleep(1);
  //verifica se tem valor de resposta pronto, em caso afirmativo envia o valor da leitura de volta 
  if(deviceConnected && rxValue == "L" && valorRes != ""){
    characteristicTX->setValue(valorRes);
    characteristicTX->notify();

    rxValue = "";
    valorRes = "";
    clearSerial();
  }
}
