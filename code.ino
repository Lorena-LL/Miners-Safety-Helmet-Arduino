/*
  MQ-2
    "in medii domestice este considerat ca o concentratie de peste 1,5% de gaz metan este periculoasa"
    Dar eu am considerat orice valoare mai mare de 0ppm CH4 periculoasa deoarece galeriile subterane nu sunt locuri cu o aerisire buna si o explozie ar aduce daune foarte mari

    "Valorile limită pentru concentrații de CO în aer sunt adesea în jur de 9 ppm.
    La niveluri de 50 ppm sau mai mult, pot apărea simptomele, cum ar fi dureri de cap, amețeli și greață.""
    Dar eu am considerat orice valoare mai mare de 0ppm de CO periculoasa deoarece minerii sunt expusi un termen mai indelungat(mai mult de 15 min la acea concentratie mica),
    lucru care poate duce la simptomele amintite mai sus
*/

/*  
  LM393 light sensor with photoresistor
    valoare analoga:
      110 intr-o camera lumonata
      <50 la lumina naturala
      <10 in lumina directa a soarelui
      600 camera intunecata             ------ valoarea pe care o aleg
      800 am nevoie de lumina neaparat
*/



// definire pinuri
#define MQ_PIN                            A0    
#define LIGHT_SENSOR_PIN                  A1
#define DHT_PIN                           12    

#define LED_RED_PIN                       13
#define BUZZER_PIN                        9
#define LED_YELLOW_PIN                    10
#define LIGHT_PIN                         11

const int motorPin1  = 5; // Pin  7 of L293
const int motorPin2  = 6;  // Pin  2 of L293


// MQ-2 part beginning
#define RL_VALUE                     (5)
#define RO_CLEAN_AIR_FACTOR           (9.83)

#define CALIBARAION_SAMPLE_TIMES     (50)
#define CALIBRATION_SAMPLE_INTERVAL   (50)
#define READ_SAMPLE_INTERVAL         (20)
#define READ_SAMPLE_TIMES            (5)

#define GAS_CO                       (1)
#define GAS_CH4                      (3)

float COCurve[3]    = {2.3, 0.72, -0.34};
float CH4Curve[3]   = {2.3, 0.45, -0.50};
float Ro = 10;

  //variables for ch4 and co ppm values(int)
int vCH4 = 0;
int vCO = 0;

float MQResistanceCalculation(int raw_adc);
float MQCalibration(int mq_pin);
float MQRead(int mq_pin);
int MQGetGasPercentage(float rs_ro_ratio, int gas_id);
int MQGetPercentage(float rs_ro_ratio, float *pcurve);
// MQ-2 part end

// DHT11 part beginning
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE    DHT11 
DHT_Unified dht(DHT_PIN, DHTTYPE);
uint32_t delayMS;
// DHT11 part end



void setup() {
  Serial.begin(9600);

  // MQ-2:
  Serial.println("Calibrating...");
  Ro = MQCalibration(MQ_PIN);
  Serial.print("Calibration is done... "); Serial.print("Ro="); Serial.print(Ro); Serial.println("kohm");

  // DHT11:
  dht.begin();
  sensor_t sensor;

  // SETARE PINURI:
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LIGHT_PIN,OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);

  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);

  digitalWrite(LED_RED_PIN,LOW);
  digitalWrite(LED_YELLOW_PIN,LOW);
  digitalWrite(LIGHT_PIN,LOW);
  digitalWrite(BUZZER_PIN,HIGH);//buzzerul functioneaza pe logica inversa(pe high e oprit)

}

void loop() {
  delay(100);

  // MQ-2
  vCO = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CO);
  vCH4 = MQGetGasPercentage(MQRead(MQ_PIN) / Ro, GAS_CH4);
  Serial.print("CO:"); Serial.print(vCO); Serial.print("ppm    CH4:"); Serial.print(vCH4); Serial.print("ppm    ");

  // DHT11
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  float vTemp = 0.0;
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    vTemp = event.temperature;
    Serial.print(F("Temperature: ")); Serial.print(vTemp); Serial.print(F("°C    "));
  }

  // LM393 light sensor
  unsigned int vLightSens=analogRead(LIGHT_SENSOR_PIN);   //val=0 (cel mai luminos); val=65,535(cal mai intunecos)   
  Serial.print("Intensity=");   Serial.println(vLightSens);

  // DECISION MAKING:
  digitalWrite(BUZZER_PIN,HIGH);

  if(vCO<0 || vCH4<0){  //daca depaseste valoarea de 32,767 ~ 3,3% concentratie de gaz in aer 
    digitalWrite(LED_RED_PIN,HIGH);
    tone(BUZZER_PIN, 600);
    delay(150);
    digitalWrite(LED_RED_PIN,LOW);
    tone(BUZZER_PIN, 250);
    delay(150);
  }
  else if(vCO>0 || vCH4>1){
    digitalWrite(LED_RED_PIN,HIGH);
    tone(BUZZER_PIN, 600);
    delay(100);
    digitalWrite(LED_RED_PIN,LOW);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(50);
  }
  else{
    digitalWrite(LED_RED_PIN,LOW);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN,HIGH);

    if (!isnan(event.temperature)) {// daca s-a citit corect temperatura
      vTemp = event.temperature;

      if(vTemp>25.0 || vTemp<-5.0){         // pentru temperatura mai mare de 30 de grade gelsius sau mai mica de -5 garde celsius atentioneaza cu buzzer si led galben
        digitalWrite(LED_YELLOW_PIN,HIGH);
        tone(BUZZER_PIN, 1000);
        delay(200);
        digitalWrite(LED_YELLOW_PIN,LOW);
        noTone(BUZZER_PIN);
        digitalWrite(BUZZER_PIN,HIGH);
        delay(500);
        if(vTemp>25.0){
          digitalWrite(motorPin1, HIGH);
          // digitalWrite(motorPin2, LOW);
        }
      }
      else{
        digitalWrite(LED_YELLOW_PIN,LOW);
        noTone(BUZZER_PIN);
        digitalWrite(BUZZER_PIN,HIGH);
        digitalWrite(motorPin1, LOW);
      }
    }

    if(vLightSens > 600){  // decision for turning on(or not) the helmet light
      digitalWrite(LIGHT_PIN,HIGH);        //if light is not present,LED on
    }
    else{
      digitalWrite(LIGHT_PIN,LOW);         //if light is present,LED off
    }

  }
}



////////// FUNCTII pentru MQ-2:

float MQResistanceCalculation(int raw_adc) {
  return (((float)RL_VALUE * (1023 - raw_adc) / raw_adc));
}

float MQCalibration(int mq_pin) {
  int i;
  float val = 0;

  for (i = 0; i < CALIBARAION_SAMPLE_TIMES; i++) {
    val += MQResistanceCalculation(analogRead(mq_pin));
    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  val = val / CALIBARAION_SAMPLE_TIMES;

  val = val / RO_CLEAN_AIR_FACTOR;

  return val;
}

float MQRead(int mq_pin) {
  int i;
  float rs = 0;

  for (i = 0; i < READ_SAMPLE_TIMES; i++) {
    rs += MQResistanceCalculation(analogRead(mq_pin));
    delay(READ_SAMPLE_INTERVAL);
}

  rs = rs / READ_SAMPLE_TIMES;

  return rs;
}

int MQGetGasPercentage(float rs_ro_ratio, int gas_id) {
  if (gas_id == GAS_CO) {
    return MQGetPercentage(rs_ro_ratio, COCurve);
  } else if (gas_id == GAS_CH4) {
    return MQGetPercentage(rs_ro_ratio, CH4Curve);
  }
  return 0;
}

int MQGetPercentage(float rs_ro_ratio, float *pcurve) {
  return (pow(10, ((log(rs_ro_ratio) - pcurve[1]) / pcurve[2]) + pcurve[0]));
}


