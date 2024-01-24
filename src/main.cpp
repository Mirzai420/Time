#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <RotaryEncoder.h>
#include <ClockScreen.h>
#include <Debug.h>
#include <EinschaltenScreen.h>
#include <driver/ledc.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>


AsyncWebServer server(80);


const char* ssid = "MRM";       // SSID deines WLANs
const char* password = "3dm3tsr3d178u"; // Passwort deines WLANs

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

#define CLK_PIN 26
#define DT_PIN 27
#define SW_PIN 33
#define SUMMER_PIN 19
#define TIMER_PIN 12

#define SCREEN_EINSCHALTEN 0
#define SCREEN_HOME 1
#define SCREEN_MENU 2
#define SCREEN_SELECTED_TIME 3
#define SCREEN_EINWILLIGUNG 4
#define SCREEN_INDIVIDUAL_TIMER 5

int selectedHour1 = 0, selectedMinute1 = 0, selectedSecond1 = 0;
int selectedHour2 = 0, selectedMinute2 = 0, selectedSecond2 = 0;
unsigned long timerEndTime1 = 0, timerEndTime2 = 0;
bool isSelectedHour1 = false, isSelectedMinute1 = false, isSelectedSecond1 = false;
bool isSelectedHour2 = false, isSelectedMinute2 = false, isSelectedSecond2 = false;

RotaryEncoder encoder(CLK_PIN, DT_PIN);

#define MENU_ANZ 10

RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ClockScreen clockScreen(display, rtc);
EinschaltenScreen einschaltenScreen(display);

const char* menu[MENU_ANZ] = {
  "Eier",
  "Pizza",
  "Ente",
  "Ofen",
  "Soße",
  "Kuchen",
  "Kaffee",
  "Schokolade",
  "Waffel",
  "Stoppuhr",
};

struct MenuEntry {
  int hour;
  int minute;
  int second;
};

MenuEntry menuTimes[MENU_ANZ];

int currentScreen = SCREEN_EINSCHALTEN;
int selectedMenuItem = 0;
bool isSecondTimerActive = false;
int Selection = 0;
int Selection2 = 0;

unsigned long lastButtonPress = 0;
unsigned long lastMenuPress = 0;
unsigned long lastSelectedTimePress = 0;
unsigned long einschaltenStartTime = 0;
unsigned long lastActivityTime = 0;

unsigned long lastPin33Press = 0;  
unsigned long lastEinwilligungPress = 0;

bool someOtherConditionForTimer2 = false;

int lastEncoderPosition = 0;

int sensitivity = 1;

int selectedHour = 0;
int selectedMinute = 0;
int selectedSecond = 0;

int storedHour = 0;
int storedMinute = 0;
int storedSecond = 0;

bool isSelectedHour = true;
bool isSelectedMinute = false;
bool isSelectedSecond = false;

bool isTimerRunning = false;
unsigned long timerEndTime;
bool isSummerActive = false;

unsigned long selectedTime = 0;
unsigned long selectedTimeStored = 0;

int btn_pressed = 0;
int btn_counter = 0;
int tmrbtn_pressed = 0;
int tmrbtn_counter = 0;



unsigned long selectedMenuTime = 0;

void handleMenu(AsyncWebServerRequest *request) {
  // Erstellen Sie ein JSON-Objekt für die Menüeinträge
  DynamicJsonDocument jsonMenu(1024);

  // Fügen Sie die Menüeinträge zum JSON-Objekt hinzu
  JsonArray menuArray = jsonMenu.createNestedArray("menu");
  for (int i = 0; i < MENU_ANZ; i++) {
    JsonObject menuItem = menuArray.createNestedObject();
    menuItem["name"] = menu[i];
    menuItem["hour"] = menuTimes[i].hour;
    menuItem["minute"] = menuTimes[i].minute;
    menuItem["second"] = menuTimes[i].second;
  }
String jsonString;
  serializeJson(jsonMenu, jsonString);

  // Senden Sie den JSON-String als Antwort auf die Anfrage
  request->send(200, "application/json", jsonString);
}

void handleRoot(AsyncWebServerRequest *request) {
  String html = "<html><body>";
  html += "<h1>Hello from your Arduino!</h1>";
  html += "</body></html>";
  request->send(200, "text/html", html);
  request->send(200, "text/plain", "Hello, world!");
}
void showClock();
void showIndividualTime();
void showSelectedTimer();
void showEinwilligungScreen();
void startTimer();
void resetTimer();
void checkEncoderPosition();
void showMenu();
void updateSelectedValue();
void handleMenuItemSelection(int menuItemIndex);
void updateSelectedTime();
void printDebugInfo();
int detect_button_press();
int detect_timer_button_press();

void setup() {
  Serial.begin(9600);

  // Verbindung zum WLAN herstellen
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Verbindung zum WLAN wird hergestellt...");
  }

  Serial.println("Erfolgreich mit dem WLAN verbunden");

  // Set up the web server
  server.on("/", HTTP_GET, handleRoot);
  server.begin();

  server.on("/menu", HTTP_GET, handleMenu);

  // Initialisierung des LEDC
  ledcSetup(0, 1000, 10);  // Kanal 0, Frequenz 1000 Hz, 10-Bit-Auflösung
  ledcAttachPin(SUMMER_PIN, 0);

  pinMode(SW_PIN, INPUT_PULLUP);
  pinMode(TIMER_PIN, INPUT_PULLDOWN);
  pinMode(SUMMER_PIN, OUTPUT);
  noTone(SUMMER_PIN);

  if (!rtc.begin()) {
    DEBUG_PRINTLN("Couldn't find RTC");
    while (1);
  }

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    DEBUG_PRINTLN(F("SSD1306 allocation failed"));
    for (;;);
  }

  currentScreen = SCREEN_EINSCHALTEN;
  einschaltenScreen.startScreen();

  currentScreen = SCREEN_HOME;
  showClock();

  for (int i = 0; i < MENU_ANZ; i++) {
    menuTimes[i].hour = 0;
    menuTimes[i].minute = 0;
    menuTimes[i].second = 0;
  }
}

void handleMenuItemSelection(int menuItemIndex) {
  menuTimes[menuItemIndex].hour = selectedHour;
  menuTimes[menuItemIndex].minute = selectedMinute;
  menuTimes[menuItemIndex].second = selectedSecond;

  selectedMenuTime = selectedTime;
}

void updateSelectedTime() {
  if (currentScreen == SCREEN_SELECTED_TIME) {
    selectedTime = selectedHour * 3600000UL + selectedMinute * 60000UL + selectedSecond * 1000UL;

    // Aktualisiere die ausgewählte Zeit nur für den aktuellen Menüpunkt
    menuTimes[selectedMenuItem].hour = selectedHour;
    menuTimes[selectedMenuItem].minute = selectedMinute;
    menuTimes[selectedMenuItem].second = selectedSecond;
    selectedTimeStored = selectedTime;
  }
}


void updateSelectedValue() {
  if (isSelectedHour) {
    if (selectedHour < 99) {
      selectedHour++;
    } else {
      selectedHour = 0;
    }
    showIndividualTime();
  } else if (isSelectedMinute) {
    if (selectedMinute < 59) {
      selectedMinute++;
    } else {
      selectedMinute = 0;
    }
    showIndividualTime();
  } else if (isSelectedSecond) {
    if (selectedSecond < 59) {
      selectedSecond++;
    } else {
      selectedSecond = 0;
    }
    showIndividualTime();
  }
}

int detect_button_press()
{
  if (LOW == digitalRead(SW_PIN))
  {
    btn_pressed = 1;
  }
  else
  {
    if (btn_pressed)
    {
      btn_counter++;
      btn_pressed = 0;
      return true;
    }
  }
  return false;
}

int detect_timer_button_press()
{
  if (HIGH == digitalRead(TIMER_PIN))
  {
    tmrbtn_pressed = 1;
  }
  else
  {
    if (tmrbtn_pressed)
    {
      tmrbtn_counter++;
      tmrbtn_pressed = 0;
      return true;
    }
  }
  return false;
}



void showClock() {
  display.clearDisplay();
  clockScreen.show();
  display.display();
}

void updateFixedTimeBasedOnMenu();
// Feste Zeiten für jeden Menüeintrag (in Millisekunden)
unsigned long fixedTimes[MENU_ANZ] = 
{
  300000,   // Beispiel: 5 Minuten für "Eier"
  900000,   // Beispiel: 15 Minuten für "Pizza"
  3600000,  // Beispiel: 1 Stunde für "Ente"
  180000,  // Beispiel: 3 Minuten für "Ofen"
  600000,  // Beispiel: 10 Minuten für "Soße"
  1800000,  // Beispiel: 30 Minuten für "Kuchen"
  360000,  // Beispiel: 6 Minuten für "Kaffee"
  420000,  // Beispiel: 7 Minuten für "Schokolade"
  480000,  // Beispiel: 8 Minuten für "Waffel"
  60000  // Beispiel: 1 Minute für "Stoppuhr"
};

void showMenu() {
  display.clearDisplay();
  display.setTextSize(1);

  int startIndex = selectedMenuItem > 5 ? selectedMenuItem - 5 : 0;

  for (int i = startIndex; i < startIndex + 6 && i < MENU_ANZ; i++) {
    if (i == selectedMenuItem && currentScreen == SCREEN_MENU) {
      display.setTextColor(BLACK, WHITE);
    } else {
      display.setTextColor(WHITE);
    }

    display.setCursor(0, (i - startIndex) * 10);
    display.print(menu[i]);

    // Anzeigen der zugehörigen Uhrzeit (rechts)
    display.setCursor(64, (i - startIndex) * 10);

    // Überprüfen Sie, ob der Index innerhalb des gültigen Bereichs liegt
    if (i < MENU_ANZ) {
      // Anzeigen der vordefinierten Zeit, unabhängig von benutzerdefinierten Zeiten
      unsigned long hours = fixedTimes[i] / 3600000;
      unsigned long minutes = (fixedTimes[i] % 3600000) / 60000;
      unsigned long seconds = (fixedTimes[i] % 60000) / 1000;

      display.print(hours);
      display.print("h ");
      display.print(minutes);
      display.print("m ");
      display.print(seconds);
      display.print("s");
    }
  }

  display.display();
}






void showSelectedTimer() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(0, 0);
  display.print(menu[selectedMenuItem]);  

  // Umrechnung der ausgewählten Zeit in Stunden, Minuten und Sekunden
  unsigned long selectedHour = fixedTimes[selectedMenuItem] / 3600000;
  unsigned long selectedMinute = (fixedTimes[selectedMenuItem] % 3600000) / 60000;
  unsigned long selectedSecond = (fixedTimes[selectedMenuItem] % 60000) / 1000;

  display.setCursor(0, 10);
  display.print(selectedHour);
  display.print("h ");
  display.print(selectedMinute);
  display.print("m ");
  display.print(selectedSecond);
  display.print("s ");

  display.display();
}


void showEinwilligungScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(22, 10);
  display.print("Willst du den");

  display.setCursor(22, 20);
  display.print("Timer starten?");

  display.setCursor(30, 40);

  int encoderPosition = encoder.getPosition();
  if (Selection == 0) {
    display.print("[Ja] - Nein");
  } else if (Selection == 1){
    display.print("Ja - [Nein]");
  }
  lastEncoderPosition = encoderPosition;
  display.display();
}

void showIndividualTime() {
  if (!isTimerRunning) {
    noTone(SUMMER_PIN);
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  if (Selection2 == 0){
  display.setCursor(0, 0);
  display.print(isSelectedHour ? "[H]" : " H. ");
  }
  else if (Selection2 == 1){
  display.setCursor(40, 0);
  display.print(isSelectedMinute ? "[M]" : " M. ");
  }
  else if (Selection2 == 2){
  display.setCursor(80, 0);
  display.print(isSelectedSecond ? "[S]" : " S. ");
  }
  display.setCursor(0, 30);
  display.print(selectedHour <= 99 ? "0" : "");
  display.print(selectedHour);

  display.setCursor(45, 30);
  display.print(selectedMinute <= 59 ? "0" : "");
  display.print(selectedMinute);

  display.setCursor(90, 30);
  display.print(selectedSecond <= 59 ? "0" : "");
  display.print(selectedSecond);

  display.display();
}

void startTimer() {
  isTimerRunning = true;
  timerEndTime = millis() + fixedTimes[selectedMenuItem];
}

void resetTimer() {
  isTimerRunning = false;
}


void checkEncoderPosition() {
  int encoderPosition = encoder.getPosition();

  if (currentScreen == SCREEN_MENU) {
    if (encoderPosition > lastEncoderPosition) {
      // Scrolling nach unten
      selectedMenuItem = (selectedMenuItem + 1) % MENU_ANZ;
      showMenu();
    } else if (encoderPosition < lastEncoderPosition) {
      // Scrolling nach oben
      selectedMenuItem = (selectedMenuItem - 1 + MENU_ANZ) % MENU_ANZ;
      showMenu();
    }

    lastEncoderPosition = encoderPosition;
  } 
  if (currentScreen == SCREEN_INDIVIDUAL_TIMER) {
    if (encoderPosition > lastEncoderPosition) {
      // Rechtsdrehung: Inkrementiere die ausgewählte Zeit
      if (isSelectedHour){
        Selection2 = (Selection2 + 1) % 3;
        showIndividualTime();
      }
      if  (isSelectedMinute){
        Selection2 = (Selection2 +1) % 3;
        showIndividualTime();
      }
      if (isSelectedSecond){
        Selection2 = (Selection2 +1) % 3;
        showIndividualTime();
      }
      
      if (isSelectedHour1) {
        if (selectedHour < 99) {
          selectedHour++;
        } else {
          selectedHour = 0;
        }
      } else if (isSelectedMinute1) {
        if (selectedMinute < 59) {
          selectedMinute++;
        } else {
          selectedMinute = 0;
        }
      } else if (isSelectedSecond1) {
        if (selectedSecond < 59) {
          selectedSecond++;
        } else {
          selectedSecond = 0;
        }
      }
    } else if (encoderPosition < lastEncoderPosition) {
      // Linksdrehung: Dekrementiere die ausgewählte Zeit
      if (isSelectedHour1) {
        if (selectedHour > 0) {
          selectedHour--;
        } else {
          selectedHour = 99;
        }
      } else if (isSelectedMinute1) {
        if (selectedMinute > 0) {
          selectedMinute--;
        } else {
          selectedMinute = 59;
        }
      } else if (isSelectedSecond1) {
        if (selectedSecond > 0) {
          selectedSecond--;
        } else {
          selectedSecond = 59;
        }
      }
      if (isSelectedHour){
        Selection2 = (Selection2 - 1) % 3;
        showIndividualTime();
      }
      if  (isSelectedMinute){
        Selection2 = (Selection2 - 1 + 3) % 3;
        showIndividualTime();
      }
      if (isSelectedSecond){
        Selection2 = (Selection2 - 1 + 3) % 3;
        showIndividualTime();
      }
    }

    lastEncoderPosition = encoderPosition;
  }
  if (currentScreen == SCREEN_EINWILLIGUNG){
    if (encoderPosition > lastEncoderPosition) {
      Selection = 0;
      showEinwilligungScreen();
    }
    else if (encoderPosition < lastEncoderPosition) {
      Selection = 1;
      showEinwilligungScreen();
    }

  }

}

void loop() {
  
  // Überprüfen des WiFi-Status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Verbindung zum WLAN wurde getrennt. Erneuter Verbindungsversuch...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Verbindung zum WLAN wird hergestellt...");
    }

    Serial.println("Erfolgreich mit dem WLAN verbunden");
  }

  
  encoder.tick();

  // Überprüfen, ob der Timer abgelaufen ist
  if (isTimerRunning && millis() >= timerEndTime) {
    resetTimer();
    showMenu();
  }
  // Überwachen der Position des Drehgebers
  int encoderPosition = encoder.getPosition();
  checkEncoderPosition();

  // Aktualisieren der festen Zeiten basierend auf der ausgewählten Zeit

  unsigned long swPressDuration = millis() - lastPin33Press;

  // Überprüfen, ob der SW_PIN (Pin 33) gedrückt wurde
  if (digitalRead(SW_PIN) == LOW) {
    // Button ist gedrückt
    if (swPressDuration >= 3000) {
      // Button wurde 4 Sekunden lang gedrückt
      currentScreen = SCREEN_EINSCHALTEN;
      showClock();
      lastPin33Press = millis();  // Zurücksetzen der Zeit für den nächsten Druck
    }
  } else {
    // Button ist nicht gedrückt
    lastPin33Press = millis();  // Zurücksetzen der Zeit, wenn der Button losgelassen wird
  }

  
  
  if (detect_button_press()) 
  {
    if (currentScreen == SCREEN_EINSCHALTEN) {
      if (millis() - einschaltenStartTime > 3000) {
        currentScreen = SCREEN_HOME;
        resetTimer();
        showClock();
      }
      } 
    else if (currentScreen == SCREEN_HOME) {
    currentScreen = SCREEN_MENU;
    showMenu();
    } 
    else if (currentScreen == SCREEN_MENU) {
    currentScreen = SCREEN_EINWILLIGUNG;
    showEinwilligungScreen();
    } 
    else if (currentScreen == SCREEN_EINWILLIGUNG) {
      if (Selection == 0) {
      currentScreen = SCREEN_SELECTED_TIME;
      showSelectedTimer();
      } 
      else if (Selection == 1) {
      currentScreen = SCREEN_MENU;
      showMenu();
      }
    }
    else if (currentScreen == SCREEN_INDIVIDUAL_TIMER){
      if (isSelectedHour) {
        isSelectedHour1 = true;
        isSelectedHour = false;
      }
      else if (isSelectedMinute){
        isSelectedMinute1 = true;
        isSelectedMinute = false;
      }
      else if (isSelectedSecond){
        isSelectedSecond1 = true;
        isSelectedSecond = false;
      }
      else if (isSelectedHour1){
        isSelectedHour1 = false;
        isSelectedHour = true;
      }
      else if (isSelectedMinute1){
        isSelectedMinute1 = false;
        isSelectedMinute = true;
      }
      else if (isSelectedSecond1){
        isSelectedSecond1 = false;
        isSelectedSecond = true;
      }
      
    }
  }
  if (detect_timer_button_press()) {
    currentScreen = SCREEN_INDIVIDUAL_TIMER;
    showIndividualTime();
  }
  if (currentScreen == SCREEN_SELECTED_TIME) {
    showSelectedTimer();
  } 
  
  if (isTimerRunning && millis() >= timerEndTime) {
    resetTimer();
    showMenu(); // Oder eine andere geeignete Aktion nach Ablauf des Timers
  }
  // Debug-Informationen (falls erforderlich)
  printDebugInfo();
  
  
}

void printDebugInfo() {
  // Debug information (if needed)
  static unsigned long lastDebugPrint = 0;
  static unsigned long lastPin25Print = 0;

  if (millis() - lastDebugPrint > 500) {
    lastDebugPrint = millis();

    DEBUG_PRINT("CLK_PIN state: ");
    DEBUG_PRINTLN(digitalRead(CLK_PIN));

    DEBUG_PRINT("DT_PIN state: ");
    DEBUG_PRINTLN(digitalRead(DT_PIN));

    // Anzeigen der Encoder-Position
    DEBUG_PRINT("Encoder Position: ");
    DEBUG_PRINTLN(encoder.getPosition());
  }

  if (millis() - lastPin25Print > 500) {
    lastPin25Print = millis();

    if (digitalRead(33) == LOW  ) {
      DEBUG_PRINTLN("Pin 33 pressed: 1");
      // Wenn Pin 33 gedrückt wird, wechsle von SCREEN_HOME zu SCREEN_MENU
      lastActivityTime = millis();
    } else {
      DEBUG_PRINTLN("Pin 33 not pressed: 0");
    }
    if (digitalRead(12) == HIGH) {
      DEBUG_PRINTLN("TIMER_PIN pressed: 1");
    } else {
      DEBUG_PRINTLN("TIMER_PIN not pressed: 0");
    }
  }
}