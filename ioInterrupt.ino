void ioInterrupt() { // This routine is in its own interrupt so it can be handled much faster
  unsigned long scanStartINT = micros(); // Start the timer to record the execution time for this interrupt
  inputVoltageRAW1023 = analogRead(inputVoltagePIN); // Read raw input voltage for analog pin, Raw values (between 0-1023)
  if (paramLoaded == true) {
    if (inputVoltageRAW1023 < idleVoltageRAW1023) {  // Take the lowest seen inputVoltageRAW1023 (aka idleVoltage)
      if (inputVoltageRAW1023 >= minVoltageRAW1023) {  // Unless it is lower than the hard coded minVoltageRAW1023
        idleVoltageRAW1023 = inputVoltageRAW1023;
      } else {
        idleVoltageRAW1023 = minVoltageRAW1023;
      }
    }
    loadPoint = map(inputVoltageRAW1023, idleVoltageRAW1023, maxVoltageRAW1023, 1, 175); // Squish loadpoints of 1-175 into the defined range of minVoltageRAW1023 and maxVoltageRAW1023

    if (inputVoltageRAW1023 < idleVoltageRAW1023) { // Keep the loadpoints within range when going below or above the limits
      loadPoint = 1;
    } else if (inputVoltageRAW1023 > maxVoltageRAW1023) {
      loadPoint = 175;
    }

    outputVoltageRAW1023 = inputVoltageRAW1023 + bias[loadPoint]; // Calculate the output voltage raw

    if (outputVoltageRAW1023 < idleVoltageRAW1023) { // Bind output voltage to minVoltageRAW1023 if it goes below (Shouldn't need this when taking realtime idle)
      outputVoltageRAW1023 = idleVoltageRAW1023;
    } else if (outputVoltageRAW1023 > maxVoltageRAW1023) {  // Bind output voltage to maxVoltageRAW1023 if it goes above
      outputVoltageRAW1023 = maxVoltageRAW1023;
    }
    
    Timer1.pwm(outputVoltagePIN, outputVoltageRAW1023); // Output the voltage!!!

  } else {

    if (inputVoltageRAW1023 <= maxVoltageRAW1023) { // Push through the voltage 1:1 if the parameters are not loaded as long as it still doesn't hit the maxVoltage
      Timer1.pwm(outputVoltagePIN, outputVoltageRAW1023);
    } else {
      Timer1.pwm(outputVoltagePIN, maxVoltageRAW1023);
    }
  }

  unsigned long scanEndINT = micros(); // Stop the timer for the interrupt execution time
  scanTimeINT = scanEndINT - scanStartINT; // Store the interrupt execution time in a variable for displaying in main loop

}
