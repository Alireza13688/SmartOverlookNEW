#include <SoftwareSerial.h>

// HC-05 TXD -> Arduino D10
// HC-05 RXD <- Arduino D11 through a voltage divider
SoftwareSerial bluetooth(10, 11);

// Eight relay outputs (Active LOW)
const byte outputPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// Two physical switch poles
const byte switch1 = A0;
const byte switch2 = A1;

int oldSwitch1;
int oldSwitch2;

String command = "";

void setup() {
  Serial.begin(9600);
  bluetooth.begin(9600);

  // Initialize all outputs OFF
  for (byte i = 0; i < 8; i++) {
    pinMode(outputPins[i], OUTPUT);
    digitalWrite(outputPins[i], HIGH);
  }

  // Initialize physical switches
  pinMode(switch1, INPUT_PULLUP);
  pinMode(switch2, INPUT_PULLUP);

  oldSwitch1 = digitalRead(switch1);
  oldSwitch2 = digitalRead(switch2);

  // Match first two outputs to the physical switches
  digitalWrite(outputPins[0], oldSwitch1);
  digitalWrite(outputPins[1], oldSwitch2);

  bluetooth.println("SMART_OVERLOOK_READY");
}

void loop() {

  // Physical switch 1 controls output 1
  int s1 = digitalRead(switch1);

  if (s1 != oldSwitch1) {
    delay(50);
    s1 = digitalRead(switch1);

    if (s1 != oldSwitch1) {
      digitalWrite(outputPins[0], s1);
      oldSwitch1 = s1;

      if (s1 == LOW)
        bluetooth.println("STATE,1,ON");
      else
        bluetooth.println("STATE,1,OFF");
    }
  }

  // Physical switch 2 controls output 2
  int s2 = digitalRead(switch2);

  if (s2 != oldSwitch2) {
    delay(50);
    s2 = digitalRead(switch2);

    if (s2 != oldSwitch2) {
      digitalWrite(outputPins[1], s2);
      oldSwitch2 = s2;

      if (s2 == LOW)
        bluetooth.println("STATE,2,ON");
      else
        bluetooth.println("STATE,2,OFF");
    }
  }

  // Receive Bluetooth commands
  while (bluetooth.available() > 0) {
    char c = bluetooth.read();

    if (c == '\n' || c == '\r') {
      if (command.length() > 0) {

        Serial.println(command);

        // Android requests the current state of all outputs
        if (command == "GET_STATE") {
          for (byte i = 0; i < 8; i++) {
            bluetooth.print("STATE,");
            bluetooth.print(i + 1);
            bluetooth.print(",");
            if (digitalRead(outputPins[i]) == LOW)
              bluetooth.println("ON");
            else
              bluetooth.println("OFF");
          }
        }

        // Turn all outputs OFF
        else if (command == "ALL_OFF") {
          for (byte i = 0; i < 8; i++) {
            digitalWrite(outputPins[i], HIGH);
            bluetooth.print("STATE,");
            bluetooth.print(i + 1);
            bluetooth.println(",OFF");
          }

          // Do not immediately re-apply the current switch positions
          oldSwitch1 = digitalRead(switch1);
          oldSwitch2 = digitalRead(switch2);

          bluetooth.println("OK,ALL_OFF");
        }

        // Control one output
        else if (command.startsWith("OUT")) {
          int number = command.substring(3, 4).toInt();

          if (number >= 1 && number <= 8) {

            if (command == "OUT" + String(number) + "_ON") {
              digitalWrite(outputPins[number - 1], LOW);

              bluetooth.print("STATE,");
              bluetooth.print(number);
              bluetooth.println(",ON");

              bluetooth.println("OK,OUT" + String(number) + ",ON");
            }

            else if (command == "OUT" + String(number) + "_OFF") {
              digitalWrite(outputPins[number - 1], HIGH);

              bluetooth.print("STATE,");
              bluetooth.print(number);
              bluetooth.println(",OFF");

              bluetooth.println("OK,OUT" + String(number) + ",OFF");
            }

            else {
              bluetooth.println("ERROR,UNKNOWN_COMMAND");
            }
          }
          else {
            bluetooth.println("ERROR,INVALID_OUTPUT");
          }
        }

        else {
          bluetooth.println("ERROR,UNKNOWN_COMMAND");
        }

        command = "";
      }
    }
    else if (command.length() < 40) {
      command += c;
    }
    else {
      command = "";
      bluetooth.println("ERROR,TOO_LONG");
    }
  }
}
