#ifndef APPPARAMETERS_H
#define APPPARAMETERS_H


struct strAppConfig {  // starting from 252 - 511
    int16_t brakeThrs = -500;        // 4 Byte - EEPROM 252 
    int16_t accelThrs = 500;         // 1 byte - EPPROM 332
    int16_t brakeEmg = -2000;        // 4 Byte - EEPROM 256
    int16_t accelEmg = 2000;         // 4 Byte - EEPROM 260
    uint8_t accAxis = 1,              // 2Byte - EEPROM 264
            leanAxis = 1;           // 2 Byte - EEPROM 266
} appConfig;


  void loadappDefaults(){
    appConfig.brakeThrs = -500;
    appConfig.accelThrs = 500;
    appConfig.brakeEmg = -2000;
    appConfig.accelEmg = 2000;
    appConfig.accAxis = 1;
    appConfig.leanAxis = 1;
  }


#if defined(ESP32) // ARDUINO_ESP32_DEV

  /*
  void reportAppConfig(){

    DEBUG_MSG("\n[appConfig report] \n\n")
    DEBUG_MSG(" config.appWifiPower: %f\n",appConfig.appWifiPower);
    DEBUG_MSG(" config.appMinClientRSSI: %d\n",appConfig.appMinClientRSSI);
    DEBUG_MSG(" config.appOpenRetryTime: %d\n",appConfig.appOpenRetryTime);
    DEBUG_MSG(" config.appCloseTimeout: %d\n",appConfig.appCloseTimeout);
    DEBUG_MSG(" config.appSettleTime: %d\n",appConfig.appSettleTime);
    DEBUG_MSG(" config.appStopTime: %d\n",appConfig.appStopTime);
    DEBUG_MSG(" config.appAutosave: %d\n",appConfig.appAutosave);
    DEBUG_MSG(" config.appFTPuser: %s\n",appConfig.appFTPuser.c_str());
    DEBUG_MSG(" config.appFTPpwd: %s\n",appConfig.appFTPpwd.c_str());
    DEBUG_MSG(" config.appCFGpwd: %s\n",appConfig.appCFGpwd.c_str());
  
  }
  */
  void ReadAppConfig(){
    ECHO_MSG("Reading APP Config\n");
    EEPROM.getBytes("appConfig", &appConfig, sizeof(appConfig));
  }

  void WriteAppConfig(){
    ECHO_MSG("Writing APP Config\n");
    EEPROM.putBytes("appConfig", &appConfig, sizeof(appConfig));    
  }



#else 
    #error "[appParameters] platform not defined"

#endif





#endif