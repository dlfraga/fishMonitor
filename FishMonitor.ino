#include <SoftwareSerial.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// DS18B20 port / one wire bus
const int ONE_WIRE_BUS = 7;
//relay pin on the board
const int RELAY_PIN = 6;
//update interval for main logic
const long UPDATE_INTERVAL = 2000;
//target temperature.
const float TARGET_TEMP = 25;
//temperature tolerance. The program will try to maintain the temperature beetween the targetTemp +(-) tolerance 
const float TEMP_TOLERANCE = 0.5;
//water sensor offset. it's used to tune the temperature that's being read
const float SENSOR_OFFSET = 0;
// Onewire bus
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//ambient sensor
DeviceAddress ambientTempSensor;
//water sensor
DeviceAddress waterTempSensor;
//number of lcd columns
const int LCD_COLUMNS = 20;
const int LCD_LINES = 4;
const byte LCD_ADDR = 0x3F;
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLUMNS, LCD_LINES, LCD_5x8DOTS);
//is the target temperature achieved
bool tempAchieved = true;
//controls the relay state
int rlstate = LOW;
//controls how much time to wait before activating the relay
const long RELAY_GRACETIME = 10000;
//ambient temperature
float tempAmbient = 0;
//water temperature
float tempWater = 0;
//the status has changed recently?
bool statusChanged = TRUE;
//Backlight delay
const long BACKL_TIME = 90000;
// time variable for backlight control
long previousMillisBack = 0;
//used to mantain the state machine
long previousMillis = 0;

//status messages
const char STATUS_MSG_INITIAL[] = "Iniciando...";
const char STATUS_MSG_RLAY_ON[] = "Aquecedor LIGADO";
const char STATUS_MSG_RLAY_OFF[] = "Aquecedor DESLIGADO";
const char STATUS_MSG_TEMP_FAIL[] = "Possivel problema no aquecedor";
const char STATUS_MSG_SENSR_FAIL[] = "Possivel problema nos sensores";


void loop()
{	
	//main execution tree. To avoid using delay() we use a state machine
	unsigned long currentMillis = millis();
	if (currentMillis - previousMillis > UPDATE_INTERVAL) {
		previousMillis = currentMillis;		
		getAndUpdateTemperature();
		printTemperaturesOnLcd(tempAmbient, tempWater);
		printStatusOnLcd(rlstate);
		switchRelay(rlstate);
	}
}

void setup()
{
	lcd.begin();
	lcd.clear();
	//setup relay pin as output
	pinMode(RELAY_PIN, OUTPUT);
	setupSensors();
	printDefaultLCDText();
}

void setupSensors() {
	//initializes the onewire sensors
	sensors.begin();
	if (!sensors.getAddress(ambientTempSensor, 0))
	{
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("VERIFY");
		lcd.setCursor(0, 1);
		lcd.print("AMBIENT TEMP SENSOR!");
		countDown();
	}

	if (!sensors.getAddress(waterTempSensor, 1))
	{
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print("VERIFY");
		lcd.setCursor(0, 1);
		lcd.print("WATER TEMP SENSOR!");
		countDown();
	}
	//disabled the wait for conversion on the ds18. It normally adds a 1sec delay
	sensors.setWaitForConversion(false);
}

void countDown() {
	int i = 9;
	while (i > 0) {
		lcd.setCursor(9, 3);
		lcd.print(i);
		i--;
		delay(1000);		
	}
}

void printDefaultLCDText() {
	//prints the default lcd text
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("Ambiente:");
	lcd.setCursor(15, 0);
	lcd.write(223);
	lcd.setCursor(16, 0);
	lcd.print("C");
	lcd.setCursor(0, 1);
	lcd.print("Agua:");
	lcd.setCursor(11, 1);
	lcd.write(223);
	lcd.setCursor(12, 1);
	lcd.print("C");
}

void printTemperaturesOnLcd(float &ambientTemp, float &waterTemp) {
	//update the lcd with the current temperatures
	lcd.setCursor(10, 0);
	lcd.print(ambientTemp,2);
	lcd.setCursor(6, 1);
	lcd.print(waterTemp,2);
}

void switchRelay(bool rltStatus) {
	if (millis() > RELAY_GRACETIME) {
		digitalWrite(RELAY_PIN, rlstate);
	}
}

void printStatusOnLcd(bool rlStatus){
	lcd.setCursor(0, 3);
	if (rlStatus) {
		lcd.print("Relay ON ");
	}
	else {
		lcd.print("Relay OFF");
	}
	if (statusChanged) {
		lcd.backlight();		
		statusChanged = FALSE;		
	} 
	else {
		if (millis() - previousMillisBack > BACKL_TIME) {
			previousMillisBack = millis();
			lcd.noBacklight();
		}
	}
}

void getAndUpdateTemperature() {
	sensors.requestTemperatures();
	tempAmbient = sensors.getTempC(ambientTempSensor);
	tempWater = sensors.getTempC(waterTempSensor);


	//delay reactivating the relay until we are completely below the target temp
	//this is done to avoid excessive flapping of the relay
	if (tempAchieved) {
		if (tempWater < (TARGET_TEMP - TEMP_TOLERANCE)) {
			tempAchieved = false;
		}
	}
	else {
		if (tempWater < (TARGET_TEMP - TEMP_TOLERANCE) || tempWater < (TARGET_TEMP + TEMP_TOLERANCE)) {
			if (rlstate != HIGH) {
				rlstate = HIGH;
				statusChanged = TRUE;
			}
			
		}
		else {
			if (rlstate != LOW) {
				rlstate = LOW;
				tempAchieved = true;
				statusChanged = TRUE;
			}			
		}
	}
	//wait RELAY_GRACETIME time after power up before activating the relay    
}