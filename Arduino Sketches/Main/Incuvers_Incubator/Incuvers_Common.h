String PadToWidth(int source, int intSize) {
  if (String(source).length() >= intSize) {
    return String(source);
  } else {
    return String("0" + PadToWidth(source, intSize - 1));
  }
}

int freeMemory() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

String ConvertMillisToScaledReadable(long totalMillisCount, int maxLen, bool includeMillis) {
  long runningAmount;
  int pureMillis = totalMillisCount % 1000;
  runningAmount = floor(totalMillisCount / 1000);
  int seconds = runningAmount % 60;
  runningAmount = floor(runningAmount / 60);
  int minutes = runningAmount % 60;
  runningAmount = floor(runningAmount / 60);
  int hours = runningAmount % 24;
  int days = floor(runningAmount / 24);
  
  String readable = "";
  int len = 0;
  if (days > 0) {
    readable = String(PadToWidth(days, 2) + "d");
    len = 3; // can't be more than 3 as the atmega will reboot before 100 days are reached.
  }

  if (maxLen > len + 3 && (hours > 0 || days > 0)) { // if we have room and have more than zero hours remaining or if we had days, report on hours.
    readable = String(readable + PadToWidth(hours, 2) + "H");
    len = len + 3; 
  }

  if (maxLen > len + 3 && (minutes > 0 || hours > 0 || days > 0)) { // if we have room and have more than zero minutes remaining or if we had hours or days, report on minutes.
    readable = String(readable + PadToWidth(minutes, 2) + "m");
    len = len + 3; 
  }

  if (maxLen > len + 3 && (seconds > 0 || minutes > 0 || hours > 0 || days > 0)) { // if we have room and have more than zero seconds remaining or if we had minutes, hours, or days, report on seconds.
    readable = String(readable + PadToWidth(seconds, 2) + "s");
    len = len + 3; 
  }

  if (maxLen > len + 4 && includeMillis) {
    readable = String(readable + PadToWidth(pureMillis, 4));
    len = len + 3; 
  }

  return readable;
}

String ConvertMillisToReadable(long totalMillisCount) {
  return ConvertMillisToScaledReadable(totalMillisCount, 20, true);
}

String CentreStringForDisplay(String prn, int width) {
  String ret = "";
  int padding = ((width - prn.length())/2);

  for (int i = 0; i < padding; i++) {
    ret += " ";
  }
  ret += prn;
  for (int i = 0; i < (width-(padding + prn.length())); i++) {
    ret += " ";
  }

  return ret;
}

char GetIndicator(boolean enabled, boolean stepping, boolean altSymbol, boolean altBlank) {
  char r = ' ';
  
  if (altBlank) { 
    r = '-'; // For use in the serial output.
  }

  if (enabled && !stepping) {
    if (altSymbol) {
      r = '#';
    } else {
      r = '*';
    }
  } else if (enabled && stepping) {
    if (altSymbol) {
      r = '=';
    } else {
      r = '+';
    }
  }

  return r;
}
