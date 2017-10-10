#include <Arduino.h>
#include <SoftwareSerial.h>
#include <RF24.h>
#include <SPI.h>
#include <Vcc.h>

/**************************************************************************
 *          FUNKCJE
 *************************************************************************/
void radioSetup();

void getData();

void getCommand();          // Funkcja sprawdza, czy dostaliśmy jakąs komendę z tabletu
void parseCommand();        // Funkcja przypisuje odpowiednie wartości do struktury Command
void sendCommand();         // Jesli dostaliśmy jakies polecenie, wysyłamy ją do łodzi.
void displayInfo();

void sendBluetooth();

/**************************************************************************
 *          Zmienne globlane, struktury
 *
 *************************************************************************/
bool dane;
const float vccMin = 3.2;                   // Najmniejsze spodziewane napięcie baterii
const float vccMax = 4.0;                   // Największe spodziewane napięcie baterii
const float VccCorrection = 3.9 / 3.8;      // Napięcie zmierzone multimetrem / napięcie zmierzone przez Arduino

const uint8_t bufLen = 5;                   // Ilość znaków w wiadomości do wysłania
byte commandBuf[bufLen];                    // Bufor do wysłania wiadomości



float vccTrnsp;                             // Napięcie zasilania transpondera
bool isCommand;                             // czy mamy komendy do wysłania ? //i - porty on/off, p - pwm
//char commandType;


// Struktura zawierająca dane do wysłania przez blue do tabletu
struct geoloc {
    double lat;
    double lon;
    uint8_t sats;
    int timeHour;
    int timeMin;
    int timeSec;
    float vccShip; //napięcie na baterii danego urządzenia
    int azymut;
    // int temperature;
    //int32_t pressure;
};
geoloc currentLoc; // zmienna zawierająca aktualne współrzędne

struct Command{                                 // Zmienne przechowujące dane do wysłania na łodź
    bool A, B, C, D = 0;                     // 5 zmiennych do wysyłania stanów 1 lub 0
    uint8_t PWM_1, PWM_2, PWM_3 = 0;            // 3 zmienne do wysyłania stanów od 0 do 100
};
Command command;




/**************************************************************************
 *          OBIEKTY
 *************************************************************************/

RF24 radio(2, 3);                   // Obiekt do obsługi radia nRD (pin CE, pin CSE)
SoftwareSerial blueSerial(8, 9);    // Rx, Tx
Vcc vcc(VccCorrection);             // Obiekt do obliczania napięcia baterii

/**************************************************************************
 *          AKCJA !!!
 *************************************************************************/
void setup() {
    Serial.begin(9600);
    blueSerial.begin(9600);

    //Serial.println("Przed Setup radio ");
    radioSetup();
    //Serial.println("Setup radio done");
    // getData();
    //displayInfo();

};

void loop() {
    getData();
    displayInfo();
    sendBluetooth();

    // Odbieranie danych i ich wysłanie do łodzi
    getCommand();
    parseCommand();
    sendCommand();
    delay(400);
}

/**************************************************************************
 * radioSetup() - Ustawienie opcji dla modułu nRF, uruchomienie modułu
 *************************************************************************/
void radioSetup() {
    Serial.print("Radio check...");
    //Ustawienia dla radia
    const uint64_t pipeIn = 0xf0f0f0f0e1;           //odbieranie
    const uint64_t pipeOut = 0xf0f0f0f0d1;          //nadawanie
    radio.begin();
    radio.setDataRate(RF24_250KBPS);                //RF24_1MBPS,RF24_2MBPS,RF24_250KBPS
    radio.setPALevel(RF24_PA_HIGH);
    radio.setChannel(19);

    // Zmienić na 108 2,508ghz

    //radio.setAutoAck(true);
    radio.enableAckPayload();

    radio.openWritingPipe(pipeOut);                 //otwarcie rury nadawczej
    radio.openReadingPipe(1, pipeIn);               //otwarcie rurt odbiorczej

    radio.writeAckPayload(1, commandBuf, sizeof(commandBuf));
    delay(20);

    radio.startListening();

    delay(80);
    Serial.println("done.");
    return;
}

/**************************************************************************
 * getData() - Odebranie danych z łodzi
 *************************************************************************/
void getData() {
    //radio.writeAckPayload(1,commandBuf, sizeof(commandBuf));

    //! radio.startListening();
    if (radio.available()) {
        //jeśli są dostępne dane na do odbioru z łodzi
        radio.read(&currentLoc, sizeof(geoloc));

        //Serial.println("Odebrano dane z radia");
        dane = 1;

    } else {
        // Serial.println("Brak danych do odbioru");
        dane = 0;
    }

//    switch (commandType) {
//        case 'i': {
//            Serial.print("Sending ports: "); Serial.println(portBuffer,BIN);
//            radio.writeAckPayload(1,&portBuffer,sizeof(portBuffer));
//            break;
//        }
//        case 'p': {
//            Serial.print("Sending PWM: ");
//            Serial.print(commandBuf[0]);Serial.print(" ");
//            Serial.print(commandBuf[1]);Serial.print(" ");
//            Serial.print(commandBuf[2]);Serial.println(" ");
//            radio.writeAckPayload(1,commandBuf, sizeof(commandBuf));
//            memset(&commandBuf,0, sizeof(commandBuf));
//
//        }
//        case 'n':{
//            //Serial.println("Brak komend");
//            break;
//        }
//        default :{
//            Serial.println("COMMAND INGNORED");
//            break;
//        }
//    }
    //  commandType = 'n';
    vccTrnsp = vcc.Read_Volts();
};

/**************************************************************************
 * displayInfo() - Wyświetlenie danych na Serial Monitor
 *************************************************************************/
void displayInfo() {
    // Informacje o napięciu zasilania transpondera (modułu bluetotoh)
    Serial.print("VT:");
    Serial.print(vccTrnsp);
    Serial.print(";");

    if (dane) {
        // Wyświetlenie danych z czujnika temp i ciśnienia
        //Serial.print("TE:");
        //Serial.print(currentLoc.temperature);
        //Serial.print(";P:");
        // Serial.print(currentLoc.pressure);

        // Wyświetlenie położenia GPS
        Serial.print(F("LAT:"));
        Serial.print(currentLoc.lat, 9);
        //Serial.print(currentLoc.lat);
        Serial.print(F(";LON:"));
        Serial.print(currentLoc.lon, 9);
        //Serial.print(currentLoc.lon);

        // Wyświetlenie liczby satelit
        Serial.print(F(";S:"));
        Serial.print(currentLoc.sats);

        // Wyświetlenie czasu
        Serial.print(F(";T:"));
        if (currentLoc.timeHour < 10) Serial.print(F("0"));
        Serial.print(currentLoc.timeHour);
        Serial.print(F(":"));
        if (currentLoc.timeMin < 10) Serial.print(F("0"));
        Serial.print(currentLoc.timeMin);
        Serial.print(F(":"));
        if (currentLoc.timeSec < 10) Serial.print(F("0"));
        Serial.print(currentLoc.timeSec);

        //Informacje z Magnetometru
        Serial.print(F(";A:"));
        Serial.print(currentLoc.azymut);

        // Informacje o napięciu zasilania łodzi
        Serial.print(";VS:");
        Serial.print(currentLoc.vccShip);

    }

    Serial.println();
}

/**************************************************************************
 * sendBluetooth() - Wyslanie danych na blutoth serial ---> do tabletu
 *************************************************************************/
void sendBluetooth() {
    //Informacje o baterii urządzenia
    blueSerial.print(F("VT:"));

    blueSerial.print(vccTrnsp);
    blueSerial.print(";");


    if (dane) {
        // Wyświetlenie danych z czujnika temp i ciśnienia
        // blueSerial.print("TE:");
        //blueSerial.print(currentLoc.temperature);
        //blueSerial.print(";P:");
        //blueSerial.print(currentLoc.pressure);

        // Wyświetlenie położenia GPS
        blueSerial.print(F("LAT:"));
        blueSerial.print(currentLoc.lat, 9);
        //blueSerial.print(currentLoc.lat);

        blueSerial.print(F(";LON:"));
        //blueSerial.print(currentLoc.lon);
        blueSerial.print(currentLoc.lon, 9);

        // Wyświetlenie liczby satelit
        blueSerial.print(F(";S:"));
        blueSerial.print(currentLoc.sats);


        // Wyświetlenie czasu
        blueSerial.print(F(";T:"));
        if (currentLoc.timeHour < 10) blueSerial.print(F("0"));
        blueSerial.print(currentLoc.timeHour);
        blueSerial.print(F(":"));
        if (currentLoc.timeMin < 10) blueSerial.print(F("0"));
        blueSerial.print(currentLoc.timeMin);
        blueSerial.print(F(":"));
        if (currentLoc.timeSec < 10) blueSerial.print(F("0"));
        blueSerial.print(currentLoc.timeSec);

        //Informacje z Magnetometru
        blueSerial.print(F(";A:"));
        blueSerial.print(currentLoc.azymut);

        //Informacje o baterii na łodzi
        blueSerial.print(F(";VS:"));
        blueSerial.print(currentLoc.vccShip);


    }
    blueSerial.println(";");
}

/**************************************************************************
 * getCommand() - Odebranie polecenia z tabletu za pomocą bluetooth
 *************************************************************************/
void getCommand() {
    int i =0;
    int bytes;
    if (blueSerial.available()) {
        while (blueSerial.available()) {
            //blueSerial.readBytesUntil('\n', commandBuf, sizeof(commandBuf));
            bytes = blueSerial.read();
            if (bytes == '\n') {
                i = 0;
                commandBuf[4] = '\0';
                //memset(&commandBuf,0, sizeof(commandBuf));
            } else {
                commandBuf[i] = (byte) bytes;
                i++;
            }
        }
        // Po zakończonym odczycie wiersza, sprawdzamy czy dane są z odpowiedniego zakresu
        if(commandBuf[0]<16 && commandBuf[1]<=100 && commandBuf[2]<=100 && commandBuf[3]<=100){
            // [0] - Stan portów 0x0000 1111 (15)
            // [1] - Stan PWM 1: 0-100
            // [2] - Stan PWM 2: 0-100
            // [3] - Stan PWM 3: 0-100
            // [4] - Znak końca lini '\n'
            isCommand = true;
        } else {
            Serial.println("Command incorrect!");
            memset(&commandBuf,0, sizeof(commandBuf));
            isCommand = false;
        }
    } else isCommand = false;
}
/**************************************************************************
 *          parseCommand()
 * Odczytuje wartości z commandBuf i przypisuje je do odpowiednich
 * elementów struktury Command
 *************************************************************************/
void parseCommand(){
    if(isCommand){
        byte temp = commandBuf[0];          // stany portów 0x0000 DCBA
        command.D = (bool) (temp & 8);
        command.C = (bool) (temp & 4);
        command.B = (bool) (temp & 2);
        command.A = (bool) (temp & 1);

        // Przypisanie do struktury wartości z bufora
        command.PWM_1 = commandBuf[1];
        command.PWM_2 = commandBuf[2];
        command.PWM_3 = commandBuf[3];

        // Podgląd na Serial
//        Serial.print("Temp:");Serial.print(temp,BIN);
//        Serial.print(" D:");Serial.print(command.D);
//        Serial.print(" C:");Serial.print(command.C);
//        Serial.print(" B:");Serial.print(command.B);
//        Serial.print(" A:");Serial.print(command.A);
//        Serial.print(" P1:");Serial.print(command.PWM_1);
//        Serial.print(" P2:");Serial.print(command.PWM_2);
//        Serial.print(" P3:");Serial.println(command.PWM_3);

    } else return;
}
/**************************************************************************
 *          sendCommand()
 *          Wyslanie danych przez nRF do Łodzi
 *************************************************************************/

void sendCommand(){
    if (isCommand){
        radio.writeAckPayload(1,&command, sizeof(command));
        //memset(&commandBuf,0, sizeof(commandBuf));
        //Serial.println("Sending done.");
        isCommand = false;
    }
}



