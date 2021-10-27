/*
 * Copyright (c) 2020 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "rtos/ThisThread.h"
#include "NTPClient.h"

#include "certs.h"
#include "iothub.h"
#include "iothub_client_options.h"
#include "iothub_device_client.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/tickcounter.h"
#include "azure_c_shared_utility/xlogging.h"

#include "iothubtransportmqtt.h"

#if MBED_CONF_IOTKIT_HTS221_SENSOR == true
#include "HTS221Sensor.h"
#endif
#if MBED_CONF_IOTKIT_BMP180_SENSOR == true
#include "BMP180Wrapper.h"
#endif
#include "OLEDDisplay.h"
#include "Servo.h"

// UI
OLEDDisplay oled( MBED_CONF_IOTKIT_OLED_RST, MBED_CONF_IOTKIT_OLED_SDA, MBED_CONF_IOTKIT_OLED_SCL );

static DevI2C devI2c( MBED_CONF_IOTKIT_I2C_SDA, MBED_CONF_IOTKIT_I2C_SCL );
#if MBED_CONF_IOTKIT_HTS221_SENSOR == true
static HTS221Sensor hum_temp(&devI2c);
#endif
#if MBED_CONF_IOTKIT_BMP180_SENSOR == true
static BMP180Wrapper hum_temp( &devI2c );
#endif

// Servo2 (Pin mit PWM)
Servo servo2 ( MBED_CONF_IOTKIT_SERVO2 );

/**
 * This example sends and receives messages to and from Azure IoT Hub.
 * The API usages are based on Azure SDK's official iothub_convenience_sample.
 */

// Global symbol referenced by the Azure SDK's port for Mbed OS, via "extern"
NetworkInterface *_defaultSystemNetwork;

static bool exit_message_received = false;

static void on_connection_status(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* user_context)
{
    if (result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED) {
        LogInfo("Connected to IoT Hub");
    } else {
        LogError("Connection failed, reason: %s", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
    }
}

static IOTHUBMESSAGE_DISPOSITION_RESULT on_message_received(IOTHUB_MESSAGE_HANDLE message, void* user_context)
{
    LogInfo("Message received from IoT Hub");

    const unsigned char *data_ptr;
    size_t len;
    if (IoTHubMessage_GetByteArray(message, &data_ptr, &len) != IOTHUB_MESSAGE_OK) {
        LogError("Failed to extract message data, please try again on IoT Hub");
        return IOTHUBMESSAGE_ABANDONED;
    }

    LogInfo("Message body: %.*s", len, data_ptr);

    // exit Message beendet loop()
    if ( strncmp( (char*) data_ptr, "exit", len) == 0 ) 
    {
      exit_message_received = true;
    }

    float value;
    if  ( sscanf( (char*) data_ptr, "servo2=%f", &value ) == 1 )
        servo2 = value;

    return IOTHUBMESSAGE_ACCEPTED;
}

static void on_message_sent(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
    if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
        LogInfo("Message sent successfully");
    } else {
        LogInfo("Failed to send message, error: %s",
            MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    }
}

void loop() 
{
    uint8_t id;
    float temp, hum;
    static const char connection_string[] = MBED_CONF_APP_IOTHUB_CONNECTION_STRING;
    bool trace_on = MBED_CONF_APP_IOTHUB_CLIENT_TRACE;
    tickcounter_ms_t interval = 100;
    IOTHUB_CLIENT_RESULT res;

    LogInfo("Initializing IoT Hub client");
    IoTHub_Init();

    IOTHUB_DEVICE_CLIENT_HANDLE client_handle = IoTHubDeviceClient_CreateFromConnectionString( connection_string, MQTT_Protocol );
    if (client_handle == nullptr) {
        LogError("Failed to create IoT Hub client handle");
        return;
    }

    // Enable SDK tracing
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_LOG_TRACE, &trace_on);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to enable IoT Hub client tracing, error: %d", res);
        return;
    }

    // Enable static CA Certificates defined in the SDK
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_TRUSTED_CERT, certificates);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set trusted certificates, error: %d", res);
        return;
    }

    // Process communication every 100ms
    res = IoTHubDeviceClient_SetOption(client_handle, OPTION_DO_WORK_FREQUENCY_IN_MS, &interval);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set communication process frequency, error: %d", res);
        return;
    }

    // set incoming message callback
    res = IoTHubDeviceClient_SetMessageCallback(client_handle, on_message_received, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set message callback, error: %d", res);
        return;
    }

    // Set connection/disconnection callback
    res = IoTHubDeviceClient_SetConnectionStatusCallback(client_handle, on_connection_status, nullptr);
    if (res != IOTHUB_CLIENT_OK) {
        LogError("Failed to set connection status callback, error: %d", res);
        return;
    }

    // Send ten message to the cloud (one per second)
    // or until we receive a message from the cloud
    IOTHUB_MESSAGE_HANDLE message_handle;
    char message[80];
    int i = 0;
    while ( true )
    {
      if (exit_message_received) {
        // If we have received a exit message from the cloud, don't send more
        // messages
        break;
      }

        // Temperator Sensor lesen und als PayLoad aufbereiten
        hum_temp.read_id(&id);
        hum_temp.get_temperature(&temp);
        hum_temp.get_humidity(&hum); 
        sprintf( message, "{ \"message\": %d, \"sensorid\": 0x%X, \"temp\": %2.2f, \"hum\": %2.1f }", i, id, temp, hum );
        LogInfo("Sending: \"%s\"", message);

        message_handle = IoTHubMessage_CreateFromString(message);
        if (message_handle == nullptr) {
            LogError("Failed to create message");
            return;
        }

        oled.cursor( 1, 0 );
        oled.printf( "id  : %d\n", i++ );
        oled.cursor( 2, 0 );    
        oled.printf( "temp: %2.2f\n", temp );        
        oled.cursor( 3, 0 );    
        oled.printf( "hum : %2.2f\n", hum );        

        const char* MSG_ID = "iotkit";
        const char* MSG_CORRELATION_ID = "sensors";
        //const char* MSG_PROP_KEYS[2] = { "temp", "hum" };
        //const char* MSG_PROP_VALS[2] = { "23.0", "40.0" };

        // Sent system properties on the message
        (void)IoTHubMessage_SetMessageId(message_handle, MSG_ID);
        (void)IoTHubMessage_SetCorrelationId(message_handle, MSG_CORRELATION_ID);

        // Set the custom properties on the message
        //MAP_HANDLE mapHandle = IoTHubMessage_Properties(message_handle);
        //for (size_t index = 0; index < 2; index++)
        //{
        //    (void)Map_AddOrUpdate(mapHandle, MSG_PROP_KEYS[index], MSG_PROP_VALS[index]);
        //}        

        res = IoTHubDeviceClient_SendEventAsync(client_handle, message_handle, on_message_sent, nullptr);
        IoTHubMessage_Destroy(message_handle); // message already copied into the SDK

        if (res != IOTHUB_CLIENT_OK) {
            LogError("Failed to send message event, error: %d", res);
            break;
        }
        // Um "Daily messages quota" nicht zu ueberschreiten
        ThisThread::sleep_for(15s);
    }

    IoTHubDeviceClient_Destroy(client_handle);
    IoTHub_Deinit();
}

int main() 
{
    LogInfo("Connecting to the network");

    _defaultSystemNetwork = NetworkInterface::get_default_instance();
    if (_defaultSystemNetwork == nullptr) {
        LogError("No network interface found");
        return -1;
    }

    int ret = _defaultSystemNetwork->connect();
    if (ret != 0) {
        LogError("Connection error: %d", ret);
        return -1;
    }
    LogInfo("Connection success, MAC: %s", _defaultSystemNetwork->get_mac_address());

    LogInfo("Getting time from the NTP server");

    NTPClient ntp(_defaultSystemNetwork);
    ntp.set_server("time.google.com", 123);
    time_t timestamp = ntp.get_timestamp();
    if (timestamp < 0) {
        LogError("Failed to get the current time, error: %ld", timestamp);
        return -1;
    }
    LogInfo("Time: %s", ctime(&timestamp));

    rtc_init();
    rtc_write(timestamp);
    time_t rtc_timestamp = rtc_read(); // verify it's been successfully updated
    LogInfo("RTC reports %s", ctime(&rtc_timestamp));

    uint8_t id;
    float value1, value2;
    
    oled.clear();
    oled.printf( "Azure Cloud Demo\n" );
    
    /* Init all sensors with default params */
    hum_temp.init(NULL);
    hum_temp.enable();
    
    hum_temp.read_id(&id);
    printf("HTS221  humidity & temperature    = 0x%X\r\n", id);  

    servo2 = 0.5f;  

    LogInfo("Starting the Loop");
    loop();
    LogInfo("The Loop has ended");

    while (true) {
        sleep();
    }

    return 0;
}
