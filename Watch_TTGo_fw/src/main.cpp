#include "config.h"

// Check if Bluetooth configs are enabled
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Bluetooth Serial object
BluetoothSerial SerialBT;

// Watch objects
TTGOClass *watch;
TFT_eSPI *tft;
BMA *sensor;

volatile uint8_t state;
volatile bool irqBMA = false;
volatile bool irqButton = false;

bool sessionStored = false;
bool sessionSent = false;

//Global vars

unsigned long last = 0;
uint32_t sessionId = 30;
unsigned long updateTimeout = 0;
unsigned long distance = 0;
float stepLength = 0.7; //meters



void initHikeWatch()
{
    // LittleFS   
    if(!LITTLEFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
        Serial.println("LITTLEFS Mount Failed");
        return;
    }

    // Stepcounter
    // Configure IMU
    Acfg cfg;
    cfg.odr = BMA4_OUTPUT_DATA_RATE_200HZ;
    cfg.range = BMA4_ACCEL_RANGE_2G;
    cfg.bandwidth = BMA4_ACCEL_OSR4_AVG1;
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    //Enable bma423 with config
    sensor->accelConfig(cfg);
    sensor->enableAccel();
    
    // Set up interrupt for steps 
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] {
        irqBMA = 1; // Set interrupt to set irq value to 1
    }, RISING); 

    // Enable BMA423 step count feature
    sensor->enableFeature(BMA423_STEP_CNTR, true);

    // Reset steps
    sensor->resetStepCounter();

    // Turn on step interrupt
    sensor->enableStepCountInterrupt();

    // Side button
    pinMode(AXP202_INT, INPUT_PULLUP);
    attachInterrupt(AXP202_INT, [] {
        irqButton = true;
    }, FALLING);

    //!Clear IRQ unprocessed first
    watch->power->enableIRQ(AXP202_PEK_SHORTPRESS_IRQ, true);
    watch->power->clearIRQ();

    return;
}

void sendDataBT(fs::FS &fs, const char * path)
{
    /* Sends data via SerialBT */
    fs::File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while(file.available()){
        SerialBT.write(file.read());
    }
    file.close();
}

void sendSessionBT()
{
    // Read session and send it via SerialBT
    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    watch->tft->drawString("Sending session", 20, 80);
    watch->tft->drawString("to Hub", 80, 110);

    // Sending session id
    sendDataBT(LITTLEFS, "/id.txt");
    SerialBT.write(';');

    // Sending steps
    sendDataBT(LITTLEFS, "/steps.txt");
    SerialBT.write(';');

    // Sending distance
    sendDataBT(LITTLEFS, "/distance.txt");
    SerialBT.write(';');

    // Send connection termination char
    SerialBT.write('\n');
}



void saveIdToFile(uint16_t id)
{
    sessionStored = true; // Something is now stored
    
    char buffer[10];
    itoa(id, buffer, 10);
    writeFile(LITTLEFS, "/id.txt", buffer);
}

void saveStepsToFile(uint32_t step_count)
{
    char buffer[10];
    itoa(step_count, buffer, 10);
    writeFile(LITTLEFS, "/steps.txt", buffer);
}

void saveDistanceToFile(float distance)
{
    char buffer[10];
    itoa(distance, buffer, 10);
    writeFile(LITTLEFS, "/distance.txt", buffer);
}

void deleteSession()
{
    deleteFile(LITTLEFS, "/id.txt");
    deleteFile(LITTLEFS, "/distance.txt");
    deleteFile(LITTLEFS, "/steps.txt");
    deleteFile(LITTLEFS, "/coord.txt");
}

void setup()
{
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->openBL(); // turns on backlight

    //Receive objects for easy writing
    tft = watch->tft;
    sensor = watch->bma;
    
    initHikeWatch();

    state = 1;

    SerialBT.begin("Hiking Watch");
}

void loop()
{
    switch (state)
    {
    case 1:
    {
        /* Initial stage */
        //Basic interface
        watch->tft->fillScreen(TFT_BLACK);
        watch->tft->setTextFont(4);
        watch->tft->setTextColor(TFT_WHITE, TFT_BLACK);
        watch->tft->drawString("Hiking Watch",  45, 25, 4);
        watch->tft->drawString("Press button", 50, 80);
        watch->tft->drawString("to start session", 40, 110);

        bool exitSync = false;

        //Bluetooth discovery
        while (1)
        {
            /* Bluetooth sync */
            if (SerialBT.available())
            {
                char incomingChar = SerialBT.read();
                if (incomingChar == 'c' and sessionStored and not sessionSent)
                {
                    sendSessionBT();
                    sessionSent = true;
                }

                if (sessionSent && sessionStored) {
                    // Update timeout before blocking while
                    updateTimeout = 0;
                    last = millis();
                    while(1)
                    {
  

                        if (SerialBT.available())
                            incomingChar = SerialBT.read();
                        if (incomingChar == 'r')
                        {
                            Serial.println("Got an R");
                            // Delete session
                            deleteSession();
                            sessionStored = false;
                            sessionSent = false;
                            incomingChar = 'q';
                            exitSync = true;
                            break;
                        }

                        else if ((millis() - updateTimeout > 2000))
                        {
                            Serial.println("Waiting for timeout to expire");
                            updateTimeout = millis();
                            sessionSent = false;
                            exitSync = true;
                            break;
                        }
                    }
                }
            }
            if (exitSync)
            {
                delay(1000);
                watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                watch->tft->drawString("Hiking Watch",  45, 25, 4);
                watch->tft->drawString("Press button", 50, 80);
                watch->tft->drawString("to start session", 40, 110);
                exitSync = false;
            }

            /*      IRQ     */
            if (irqButton) {
                irqButton = false;
                watch->power->readIRQ();
                if (state == 1)
                {
                    state = 2;
                }
                watch->power->clearIRQ();
            }
            if (state == 2) {
                if (sessionStored)
                {
                    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                    watch->tft->drawString("Overwriting",  55, 100, 4);
                    watch->tft->drawString("session", 70, 130);
                    delay(1000);
                }
                break;
            }
        }
        break;
    }
    case 2:
    {
        /* Hiking session initalisation */
        
        state = 3;
        break;
    }
  case 3:
{
    /* Hiking session ongoing */
    static uint32_t lastStepCount = 0; // Initialize lastStepCount to keep track of the previous step count
    uint32_t currentStepCount;
    float currentDistance;

    // Record the current time at the start of the session
    unsigned long lastUpdate = millis();

    currentStepCount = sensor->getCounter();
    currentDistance = (currentStepCount * stepLength) / 1000.0; // Update distance

    // No need to clear the screen here if we're going to update it immediately in the loop

    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    watch->tft->drawString("Starting hike", 45, 100);
    delay(1000); // 
    
    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
    watch->tft->setCursor(45, 70);
    watch->tft->print("Steps: ");
    watch->tft->print(currentStepCount);
    watch->tft->setCursor(45, 100);
    watch->tft->print("Dist: ");
    watch->tft->print(currentDistance);
    watch->tft->print(" km");

    while (state == 3) // Keep running while in active mode
    {
            if( millis() - lastUpdate >= 1000){  // Update the step count once a second
                uint32_t previousStepCount = currentStepCount;
                currentStepCount = sensor->getCounter();
                
            // Only update the display if the current step count has increased since the last update, to prevent flickering + battery saving
                if (currentStepCount > previousStepCount) 
            {
                    currentDistance = (currentStepCount * stepLength) / 1000.0; // Update distance
                    watch->tft->fillRect(0, 0, 240, 240, TFT_BLACK);
                    watch->tft->setCursor(45, 70);
                    watch->tft->print("Steps: ");
                    watch->tft->print(currentStepCount);

                    watch->tft->setCursor(45, 100);
                    watch->tft->print("Dist: ");
                    watch->tft->print(currentDistance);
                    watch->tft->print(" km");

                lastStepCount = currentStepCount; // Update lastStepCount with the current step count
            }
            lastUpdate = millis(); // Update the last update time
        }

        if (irqButton)
        {
            irqButton = false;
            watch->power->readIRQ();
            if (watch->power->isPEKShortPressIRQ())
            {
                // End the session
                state = 4;
            }
            watch->power->clearIRQ();
        }

         delay(10); // Delay to prevent the loop from running too fast + battery saving
    }
    break;
}



    case 4:
{
    /* End of hiking session */

    // Save session data to files
    saveIdToFile(sessionId);
    saveStepsToFile(sensor->getCounter());
    saveDistanceToFile(distance);

    // Reset session variables
    sessionId++;
    sensor->resetStepCounter();

    // Change state to initial stage
    state = 1;

    break;
}
    default:
        // Restart watch
        ESP.restart();
        break;
    }
}