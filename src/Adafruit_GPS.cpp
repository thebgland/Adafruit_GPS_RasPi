#include <Adafruit_GPS.h>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <wiringPi.h>
#include <wiringSerial.h>

// how long are max NMEA lines to parse?
#define MAXLINELENGTH 120

// we double buffer: read one line in and leave one for the main program
volatile char line1[MAXLINELENGTH];
volatile char line2[MAXLINELENGTH];
// our index into filling the current line
volatile uint8_t lineidx=0;
// pointers to the double buffers
volatile char *currentline;
volatile char *lastline;
volatile bool recvdflag;
volatile bool inStandbyMode;

bool Adafruit_GPS::parse(char *nmea) {
  // do checksum check

  // first look if we even have one
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += parseHex(nmea[strlen(nmea)-2]);

    // check checksum
    for (uint8_t i=2; i < (strlen(nmea)-4); i++) {
      sum ^= nmea[i];
    }
    if (sum != 0) {
      // bad checksum :(
      return false;
    }
  }
  int32_t degree;
  long minutes;
  char degreebuff[10];
  // look for a few common sentences
  if (strstr(nmea, "$GPGGA")) {
    // found GGA
    char *p = nmea;
    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	strncpy(degreebuff, p, 2);
	p += 2;
	degreebuff[2] = '\0';
	degree = atol(degreebuff) * 10000000;
	strncpy(degreebuff, p, 2); // minutes
	p += 3; // skip decimal point
	strncpy(degreebuff + 2, p, 4);
	degreebuff[6] = '\0';
	minutes = 50 * atol(degreebuff) / 3;
	latitude_fixed = degree + minutes;
	latitude = degree / 100000 + minutes * 0.000006F;
	latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
	latitudeDegrees += int(latitude/100);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	if (p[0] == 'S') latitudeDegrees *= -1.0;
	if (p[0] == 'N') lat = 'N';
	else if (p[0] == 'S') lat = 'S';
	else if (p[0] == ',') lat = 0;
	else return false;
      }

    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	strncpy(degreebuff, p, 3);
	p += 3;
	degreebuff[3] = '\0';
	degree = atol(degreebuff) * 10000000;
	strncpy(degreebuff, p, 2); // minutes
	p += 3; // skip decimal point
	strncpy(degreebuff + 2, p, 4);
	degreebuff[6] = '\0';
	minutes = 50 * atol(degreebuff) / 3;
	longitude_fixed = degree + minutes;
	longitude = degree / 100000 + minutes * 0.000006F;
	longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
	longitudeDegrees += int(longitude/100);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	if (p[0] == 'W') longitudeDegrees *= -1.0;
	if (p[0] == 'W') lon = 'W';
	else if (p[0] == 'E') lon = 'E';
	else if (p[0] == ',') lon = 0;
	else return false;
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	fixquality = atoi(p);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	satellites = atoi(p);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	HDOP = atof(p);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	altitude = atof(p);
      }

    p = strchr(p, ',')+1;
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	geoidheight = atof(p);
      }
    return true;
  }
  if (strstr(nmea, "$GPRMC")) {
    // found RMC
    char *p = nmea;

    // get time
    p = strchr(p, ',')+1;
    float timef = atof(p);
    uint32_t time = timef;
    hour = time / 10000;
    minute = (time % 10000) / 100;
    seconds = (time % 100);

    milliseconds = fmod(timef, 1.0) * 1000;

    p = strchr(p, ',')+1;
    // Serial.println(p);
    if (p[0] == 'A')
      fix = true;
    else if (p[0] == 'V')
      fix = false;
    else
      return false;

    // parse out latitude
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	strncpy(degreebuff, p, 2);
	p += 2;
	degreebuff[2] = '\0';
	long degree = atol(degreebuff) * 10000000;
	strncpy(degreebuff, p, 2); // minutes
	p += 3; // skip decimal point
	strncpy(degreebuff + 2, p, 4);
	degreebuff[6] = '\0';
	long minutes = 50 * atol(degreebuff) / 3;
	latitude_fixed = degree + minutes;
	latitude = degree / 100000 + minutes * 0.000006F;
	latitudeDegrees = (latitude-100*int(latitude/100))/60.0;
	latitudeDegrees += int(latitude/100);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	if (p[0] == 'S') latitudeDegrees *= -1.0;
	if (p[0] == 'N') lat = 'N';
	else if (p[0] == 'S') lat = 'S';
	else if (p[0] == ',') lat = 0;
	else return false;
      }

    // parse out longitude
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	strncpy(degreebuff, p, 3);
	p += 3;
	degreebuff[3] = '\0';
	degree = atol(degreebuff) * 10000000;
	strncpy(degreebuff, p, 2); // minutes
	p += 3; // skip decimal point
	strncpy(degreebuff + 2, p, 4);
	degreebuff[6] = '\0';
	minutes = 50 * atol(degreebuff) / 3;
	longitude_fixed = degree + minutes;
	longitude = degree / 100000 + minutes * 0.000006F;
	longitudeDegrees = (longitude-100*int(longitude/100))/60.0;
	longitudeDegrees += int(longitude/100);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	if (p[0] == 'W') longitudeDegrees *= -1.0;
	if (p[0] == 'W') lon = 'W';
	else if (p[0] == 'E') lon = 'E';
	else if (p[0] == ',') lon = 0;
	else return false;
      }
    // speed
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	speed = atof(p);
      }

    // angle
    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	angle = atof(p);
      }

    p = strchr(p, ',')+1;
    if (',' != *p)
      {
	uint32_t fulldate = atof(p);
	day = fulldate / 10000;
	month = (fulldate % 10000) / 100;
	year = (fulldate % 100);
      }
    // we dont parse the remaining, yet!
    return true;
  }

  return false;
}

char Adafruit_GPS::read(void) {
  char c = 0;

  if (paused) return c;

  if(!serialDataAvail(SerialFD)) return c;
  c = serialGetchar(SerialFD);

  if (c == '\n') {
    currentline[lineidx] = 0;

    if (currentline == line1) {
      currentline = line2;
      lastline = line1;
    } else {
      currentline = line1;
      lastline = line2;
    }

    lineidx = 0;
    recvdflag = true;
  }

  currentline[lineidx++] = c;
  if (lineidx >= MAXLINELENGTH)
    lineidx = MAXLINELENGTH-1;

  return c;
}

Adafruit_GPS::Adafruit_GPS(char *device)
{
  common_init();     // Set everything to common state, then...
  SerialDevice = device;
}

void Adafruit_GPS::common_init(void) {
  recvdflag   = false;
  paused      = false;
  lineidx     = 0;
  currentline = line1;
  lastline    = line2;

    hour = minute = seconds = year = month = day =
      fixquality = satellites = 0; // uint8_t
    lat = lon = mag = 0; // char
    fix = false; // boolean
    milliseconds = 0; // uint16_t
      latitude = longitude = geoidheight = altitude =
	speed = angle = magvariation = HDOP = 0.0; // float
}

void Adafruit_GPS::begin(uint32_t baud)
{
  SerialFD = serialOpen(SerialDevice, baud);

  delay(10);
}

void Adafruit_GPS::sendCommand(const char *str) {
  serialPuts(SerialFD, str);
  serialPuts(SerialFD, "\r\n");
}


bool Adafruit_GPS::newNMEAreceived(void) {
  return recvdflag;
}

void Adafruit_GPS::pause(bool p) {
  paused = p;
}


char *Adafruit_GPS::lastNMEA(void) {
  recvdflag = false;
  return (char *)lastline;
}

uint8_t Adafruit_GPS::parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
  // if (c > 'F')
  return 0;
}

bool Adafruit_GPS::waitForSentence(const char *wait4me, uint8_t max) {
  char str[20];

  uint8_t i=0;
  while (i < max) {
    read();

    if (newNMEAreceived()) {
      char *nmea = lastNMEA();
      strncpy(str, nmea, 20);
      str[19] = 0;
      i++;

      if (strstr(str, wait4me))
	return true;
    }
  }

  return false;
}

bool Adafruit_GPS::LOCUS_StartLogger(void) {
  sendCommand(PMTK_LOCUS_STARTLOG);
  recvdflag = false;
  return waitForSentence(PMTK_LOCUS_STARTSTOPACK);
}

bool Adafruit_GPS::LOCUS_StopLogger(void) {
  sendCommand(PMTK_LOCUS_STOPLOG);
  recvdflag = false;
  return waitForSentence(PMTK_LOCUS_STARTSTOPACK);
}

bool Adafruit_GPS::LOCUS_ReadStatus(void) {
  sendCommand(PMTK_LOCUS_QUERY_STATUS);

  if (! waitForSentence("$PMTKLOG"))
    return false;

  char *response = lastNMEA();
  uint16_t parsed[10];
  uint8_t i;

  for (i=0; i<10; i++) parsed[i] = -1;

  response = strchr(response, ',');
  for (i=0; i<10; i++) {
    if (!response || (response[0] == 0) || (response[0] == '*'))
      break;
    response++;
    parsed[i]=0;
    while ((response[0] != ',') &&
	   (response[0] != '*') && (response[0] != 0)) {
      parsed[i] *= 10;
      char c = response[0];
      if (isdigit(c))
	parsed[i] += c - '0';
      else
	parsed[i] = c;
      response++;
    }
  }
  LOCUS_serial = parsed[0];
  LOCUS_type = parsed[1];
  if (isalpha(parsed[2])) {
    parsed[2] = parsed[2] - 'a' + 10;
  }
  LOCUS_mode = parsed[2];
  LOCUS_config = parsed[3];
  LOCUS_interval = parsed[4];
  LOCUS_distance = parsed[5];
  LOCUS_speed = parsed[6];
  LOCUS_status = !parsed[7];
  LOCUS_records = parsed[8];
  LOCUS_percent = parsed[9];

  return true;
}

// Standby Mode Switches
bool Adafruit_GPS::standby(void) {
  if (inStandbyMode) {
    return false;  // Returns false if already in standby mode, so that you do not wake it up by sending commands to GPS
  }
  else {
    inStandbyMode = true;
    sendCommand(PMTK_STANDBY);
    //return waitForSentence(PMTK_STANDBY_SUCCESS);  // don't seem to be fast enough to catch the message, or something else just is not working
    return true;
  }
}

bool Adafruit_GPS::wakeup(void) {
  if (inStandbyMode) {
    inStandbyMode = false;
    sendCommand("");  // send byte to wake it up
    return waitForSentence(PMTK_AWAKE);
  }
  else {
    return false;  // Returns false if not in standby mode, nothing to wakeup
  }
}
