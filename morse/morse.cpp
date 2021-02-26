#define MORSE_PIN_IN 3
#define MORSE_PIN_OUT 13

#define READY_IN 12
#define READY_OUT 11

const int DOT_DELAY = 3;
const int DASH_DELAY = DOT_DELAY * 3;
const int SPACE_DELAY = DOT_DELAY;
const int SYMBOL_DELAY = DOT_DELAY * 3;

const int ERROR_DELAY = DOT_DELAY;

typedef struct
{
	char ascii;
	String morse;
} morsePair;

morsePair table[] = {
	{'a' , ".-"    },
	{'b' , "-..."  },
	{'c' , "-.-."  },
	{'d' , "-.."   },
	{'e' , "."     },
	{'f' , "..-."  },
	{'g' , "--."   },
	{'h' , "...."  },
	{'i' , ".."    },
	{'j' , ".---"  },
	{'k' , "-.-"   },
	{'l' , ".-.."  },
	{'m' , "--"    },
	{'n' , "-."    },
	{'o' , "---"   },
	{'p' , ".--."  },
	{'q' , "--.-"  },
	{'r' , ".-."   },
	{'s' , "..."   },
	{'t' , "-"     },
	{'u' , "..-"   },
	{'v' , "...-"  },
	{'w' , ".--"   },
	{'x' , "-..-"  },
	{'y' , "-.--"  },
	{'z' , "--.."  },
	{'1' , ".----" },
	{'2' , "..---" },
	{'3' , "...--" },
	{'4' , "....-" },
	{'5' , "....." },
	{'6' , "-...." },
	{'7' , "--..." },
	{'8' , "---.." },
	{'9' , "----." },
	{'0' , "-----" },
	{' ' , "--..--"},
	{'\n', ".-.-."}
};

int tableSize = sizeof(table) / sizeof(morsePair);

enum LastChange
{
	RISE,
	FALL
};

LastChange lastChange = FALL;

String morseBuf = "";

volatile unsigned long lastChangeTs = 0; //last condition change timestamp

//from serial to morse
inline String charToMorse(char symbol) {
	for (int i = 0; i < tableSize; i++)
		if (table[i].ascii == symbol)
			return table[i].morse;
	return "";
}

inline void printDash() {
	digitalWrite(MORSE_PIN_OUT, HIGH);
	delay(DASH_DELAY);
	digitalWrite(MORSE_PIN_OUT, LOW);
}

inline void printDot() {
	digitalWrite(MORSE_PIN_OUT, HIGH);
	delay(DOT_DELAY);
	digitalWrite(MORSE_PIN_OUT, LOW);
}

inline void sendMorseCode(String morseCode) {
	for (int i = 0; i < morseCode.length(); i++) {
		switch (morseCode[i])
		{
		case '.': printDot(); break;
		case '-': printDash(); break;
		default: break;
		}
		delay(SPACE_DELAY);
	}
	delay(SYMBOL_DELAY);
}


inline void sendString(String string) {
	for (int i = 0; i < string.length(); i++) {
		String morse = charToMorse(string[i]);
		sendMorseCode(morse);
	}
}

//from morse to serial
inline char morseToChar(String morse) {
	for (int i = 0; i < tableSize; i++) {
		if (table[i].morse == morse)
			return table[i].ascii;
	}
	return '?';
}

inline void morsePinValueChangeHandler() {

	if (lastChange == FALL) {
		//value has changed from 0 to 1
		lastChange = RISE;
	}
	else if (lastChange == RISE) {
		//value has changed from 1 to 0
		unsigned long delay = millis() - lastChangeTs;

		if (delay < DOT_DELAY + ERROR_DELAY)
			morseBuf += '.';
		else if (delay < DASH_DELAY + ERROR_DELAY)
			morseBuf += '-';
		lastChange = FALL;
	}

	lastChangeTs = millis();
}

inline bool symblolIsReceived() {
	return (morseBuf != "") && (millis() - lastChangeTs > SYMBOL_DELAY) && (lastChange == FALL);
}

//"main"
void setup()
{
	pinMode(MORSE_PIN_OUT, OUTPUT);
	pinMode(READY_OUT, OUTPUT);
	pinMode(READY_IN, INPUT);

	pinMode(MORSE_PIN_IN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(MORSE_PIN_IN), morsePinValueChangeHandler, CHANGE);

	Serial.begin(9600);

	digitalWrite(READY_OUT, HIGH);
}

void loop()
{
	if (symblolIsReceived()) {

		//we received a morse code
		char symbol = morseToChar(morseBuf);
		morseBuf = "";
		Serial.print(symbol);
	}

	if (digitalRead(READY_IN) && Serial.available() > 0) {

		//we have a string to send
		digitalWrite(READY_OUT, LOW);
		String string = Serial.readString() + '\n';
		string.toLowerCase();
		sendString(string);
		digitalWrite(READY_OUT, HIGH);
	}
}