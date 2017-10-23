// ===== THINGS TO DO ==========
// 1. Check that parameters are valid signed integers.. is this required???

void readProfile() {  //This routine is an extention of the main loop and handles checking for and reading the file
  Serial.println(F(""));
  Serial.print(F("Initializing SD card"));
  if (!SD.begin(sdCardPIN)) { // Check if SD card is not there on pin 10
    Serial.println(F("\t\t...FAILED!!!"));
    return; // Then try again
  } else {
    Serial.println(F("\t\t...SUCCESS"));

    File profileInfo;
    int count = 0;
    String tempValue = "";
    //Append a string to construct the file name, then convert it to a charArray. It's messy but that's how arduino wants it.
    String fileNameSTRING = "profile";
    fileNameSTRING += profileSelect;
    fileNameSTRING += ".txt";
    char fileNameCHAR[fileNameSTRING.length() + 1]; //Create a charArray variable as long as fileName plus one
    fileNameSTRING.toCharArray(fileNameCHAR, fileNameSTRING.length() + 1); //Convert string to charArray and stuff it on the char variable
    Serial.print("Opening File:  " + fileNameSTRING);
    if (SD.exists(fileNameCHAR)) {  //Check if the file exists if so open it
      profileInfo = SD.open(fileNameCHAR);
      if (!profileInfo) { //Check if the file is opened properly, if not stop everything
        Serial.println(F("\t...FAILED!!!"));
        return;
      } else {
        Serial.println(F("\t...SUCCESS"));
        char fileChar; // need to set the file contents to char type
        Serial.print(F("Loading Parameters"));
        while (profileInfo.available()) {
          fileChar = profileInfo.read(); // Read the next character in file
          if (fileChar == '=') { //do nothing with the characters until we hit '='
            count++;
            fileChar = profileInfo.read(); // Read the next character in file
            if (fileChar == ' ') { // Do nothing with the characters until we hit a space
              while (fileChar != '\n') { // Keep reading until we hit a carrage return (TXT files do not have end of line characters, \0, so we are forced to have an end of file marker, essentially an ending line that isn't a parameter)
                fileChar = profileInfo.read(); // Read the next character in file
                tempValue.concat(fileChar); // Append the character to a temp variable
              }

              char charBuf[tempValue.length() + 1]; // Create Char variable: Again weird things in Arduino, you can't convert a String to Int, so you must convert to Char first then into String a CharArray then to an Int
              tempValue.toCharArray(charBuf, tempValue.length() + 1); // Convert string to array of chars
              bias[count] = atoi(charBuf); // Convert charArray to integer and load it into settings int Array
            } else {
              bias[count] = 0; // If nothing is found between the = and a carrage return then set a default value of 0
            }
            tempValue = ""; // Clear the temp variable for the next parameter
          }
        }
        Serial.println(F("\t\t...SUCCESS"));
        Serial.println(F(""));
        profileInfo.close();  //Close the file
        paramLoaded = true; //Lock it down so it will not be read again

        if (debug) {
          Serial.println(F("=======  READ BACK DATA  ======="));  // Read back the actual collected data from the profile file
          Serial.println(F(""));
          for (int i = 1; i <= 175; i++) {
            Serial.print(F("\tLoadPoint "));
            Serial.print(i);
            Serial.print(F(" = "));
            Serial.println(bias[i]);
          }
          Serial.println(F(""));
        }
      }
    } else {
      Serial.println(F("\t\t...FILE NOT FOUND!!!!"));
      return;
    }
  }
}
