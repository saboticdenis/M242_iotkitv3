Cloud IoT Services
==================

Cloud Anbindung an Amazon (AWS), Google und Microsoft (Azure)

## Azure IoT
***

![](images/azure-iot.png)

Quelle: https://docs.microsoft.com/de-de/azure/iot-hub/about-iot-hub
- - - 

IoT Hub ist ein in der Cloud gehosteter, verwalteter Dienst, der als zentraler Nachrichtenhub für die bidirektionale Kommunikation zwischen Ihrer IoT-Anwendung und den Geräten dient, die von der Anwendung verwaltet werden. Sie können Azure IoT Hub verwenden, um IoT-Lösungen mit zuverlässiger und sicherer Kommunikation zwischen Millionen von IoT-Geräten und einem in der Cloud gehosteten Lösungs-Back-End zu erstellen. Sie können praktisch jedes Gerät mit IoT Hub verbinden.

IoT Hub unterstützt die Kommunikation sowohl vom Gerät an die Cloud als auch von der Cloud an das Gerät. IoT Hub unterstützt mehrere Messagingmuster wie z.B. Gerät-zu-Cloud-Telemetrie, Dateiuploads von Geräten und Anforderungs-Antwort-Methoden zum Steuern der Geräte über die Cloud. Die IoT Hub-Überwachung unterstützt Sie dabei, die Integrität Ihrer Lösung sicherzustellen, indem Ereignisse wie Geräteerstellung, Geräteausfälle und Geräteverbindungen nachverfolgt werden.

Azure IoT Hub unterstützt folgende Protokolle:
* HTTPS
* AMQP
* AMQP über WebSockets
* MQTT
* MQTT über WebSockets

Das Azure IoT Beispiel wurde mit folgenden Boards getestet:
* [DISCO-L475VG-IOT01A](https://github.com/iotkitv3/intro/tree/main/hw#disco-l475vg-iot01a)
* [IoTKitV3 K64F Board](https://github.com/iotkitv3/iotkit#iotkitv3-k64f)
* [NXP K64F Original Board](https://os.mbed.com/platforms/FRDM-K64F/) via Ethernet Kabel

Das [NUCLEO-F303RE Board](https://github.com/iotkitv3/iotkit#nucleo-f303re) funktioniert nicht, wegen fehlender Unterstützung für TLS.

**Einrichten eines Azure IoT Hub-Kontos**

Befolgen Sie die offizielle Dokumentation von Azure IoT Hub zu

* Erstellen Sie einen neuen Hub im Azure-Portal ([Dokumentation](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal#create-an-iot-hub)). Für dieses Beispiel benötigen Sie einen Standard- Tier- Hub, um Cloud-to-Device-Nachrichten zu aktivieren. Dazu reicht die kostenlose Variante (Free Tier).
* Registrieren Sie ein neues Gerät bei dem von Ihnen erstellten Hub ([Dokumentation](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal#register-a-new-device-in-the-iot-hub)). Kopieren Sie den "Primary Connection String" des Geräts und übertragen diesen in `mbed_app.json` ins Feld `value` von `iothub_connection_string`.

Jetzt kann der IoT Hub mit diesem [Beispiel](main.cpp) verwendet werden.

Das [mbed Beispiel](main.cpp) verbindet sich via dem MQTT Protokoll mit der Azure Cloud. 

Dabei werden die Felder `correlation_id`, `message_id` und `payload` übertragen. Im Feld `payload` steht eine fortlaufende Id, die Id des Temperatursensors und Temperatur und Luftfeuchtigkeit (bei BPM180 Sensor: Luftdruck).

Die Events sehen in etwa so aus:

    {
        "event": {
            "origin": "K64F",
            "module": "",
            "interface": "",
            "component": "",
            "properties": {
                "system": {
                    "correlation_id": "sensors",
                    "message_id": "iotkit"
                },
                "application": {}
            },
            "annotations": {
                "iothub-connection-device-id": "K64F",
                "iothub-connection-auth-method": "{\"scope\":\"device\",\"type\":\"sas\",\"issuer\":\"iothub\",\"acceptingIpFilterRule\":null}",
                "iothub-connection-auth-generation-id": "637446820856720308",
                "iothub-enqueuedtime": 1610009322243,
                "iothub-message-source": "Telemetry",
                "x-opt-sequence-number": 985,
                "x-opt-offset": "4295386552",
                "x-opt-enqueued-time": 1610009322243
            },
            "payload": "{ \"message\": 72, \"sensorid\": 0xBC, \"temp\": 23.40, \"hum\": 36.9 }"
        }
    }

Neben dem Senden von Daten, kann das Board auch Meldungen empfangen. Dazu ist im IoT Hub zuerst die IoT Device, hier K64F, anzuwählen und dann der Tab `Message to device`. Dort können Daten an das Board gesendet werden. Im Moment wir die Ansteuerung eines Servos unterstützt.

Beispiel:

    servo2=0.2
        
Setzt den Servo 2 auf die Position 0.2.

**Anzeigen der Meldungen**

Dazu muss zuerst das Azure CLI installiert werden.

Anschliessend sich einloggen, `azure-iot` Extension installieren und Events ausgeben

    az login
    az extension add --name azure-iot
    az iot hub monitor-events --hub-name <IoT Hub Name> --props all

**Links**
* [Mbed OS example for Azure IoT Hub](https://github.com/ARMmbed/mbed-os-example-for-azure)
* [Azure C SDK](https://github.com/Azure/azure-iot-sdk-c)
* [How to test Messages arriving in Azure IoTHub](https://stackoverflow.com/questions/35381303/how-to-test-messages-arriving-in-azure-iothub)

### Azure PowerBi Pipeline

![](images/azure-pipeline.png)

- - - 

Azure stellt mit PowerBi einen Möglichkeit zur Verfügung die Sensordaten weiterzuverarbeiten.

Dazu sind einige Schritte nötig.

![](images/azure-hub.png)

Zuerst ist ein IoT Hub in der Azure Cloud zu erstellen:

![](images/azure-iotdevice.png)

Und in diesem eine IoT Device

![](images/azure-key.png)

Damit sich das IoTKitV3 mit der Azure Cloud verbinden kann, brauchen wir den "Primary Connection String". Dieser ist in die Datei `mbed_app.json` zur übertragen.
Anschliessend ist das Programm zu compilieren und z.B. wie oben beschrieben, zu testen.

![](images/azure-connect.png)

#### Auswerten der Daten 

Um die Daten in der Azure Cloud auszuwerten, braucht es einen Stream Analytics Job welcher die Daten sammelt und an PowerPi weiterreicht.

![](images/13.PNG) 

Consumer Gruppe zu IoT hub hinzufügen

![](images/14.PNG)  

Stream Analytics Job erstellen

![](images/15.PNG)  
![](images/16.PNG) 

Input und Output zum Stream Analytics Job hinzufügen und Verlinkung mit IoT Hub

![](images/17.PNG)  

Query anpassen, damit er mit dem Stream Analytics Jobs übereinstimmt. Starten des Jobs, welche die Daten sammelt und an PowerPi weiterreicht.

![](images/18.PNG)  

Zum Schluss sind die anzuzeigenden Daten in PowerPi zu erfassen und der Report zu erstellen (gelbe Markierung beachten).

![](images/19.PNG)  

Das Ergebnis sieht dann wie oben aus.

## AWS IoT
***

![](images/aws-iot.png)

Quelle: https://docs.aws.amazon.com/iot/latest/developerguide/what-is-aws-iot.html
- - -


Mit AWS IoT können Sie die am besten geeigneten und aktuellsten Technologien für Ihre Lösung auswählen. AWS IoT Core unterstützt die folgenden Protokolle, damit Sie Ihre IoT-Geräte vor Ort verwalten und unterstützen können:

* [MQTT](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html) (Message Queuing und Telemetrietransport)
* [MQTT](https://docs.aws.amazon.com/iot/latest/developerguide/mqtt.html) über WSS (Websockets Secure)
* [HTTPS](https://docs.aws.amazon.com/iot/latest/developerguide/http.html) (Hypertext Transfer Protocol - Sicher) .
* [LoRaWAN](https://docs.aws.amazon.com/iot/latest/developerguide/connect-iot-lorawan.html) (Long Range Wide Area Network)

Der AWS IoT Core-Nachrichtenbroker unterstützt Geräte und Clients, die MQTT und MQTT über WSS-Protokolle zum Veröffentlichen und Abonnieren von Nachrichten verwenden. Es werden auch Geräte und Clients unterstützt, die das HTTPS-Protokoll zum Veröffentlichen von Nachrichten verwenden.

Mit AWS IoT Core für LoRaWAN können Sie drahtlose LoRaWAN-Geräte (Low-Power-Wide Area Network mit großer Reichweite) verbinden und verwalten. AWS IoT Core für LoRaWAN ersetzt die Notwendigkeit, einen LoRaWAN Network Server (LNS) zu entwickeln und zu betreiben.

Das mbed Beispiel verbindet sich via dem MQTT Protokoll mit der AWS Cloud.

* [Mbed OS example for AWS IoT SDK](https://github.com/ARMmbed/mbed-os-example-for-aws)
* [AWS IoT Core](https://aws.amazon.com/de/iot-core/)
* [AWS SDK für C++](https://aws.amazon.com/de/sdk-for-cpp/)

## Weitere Cloud Beispiele

* [Mbed OS example for Google IoT Cloud](https://github.com/ARMmbed/mbed-os-example-for-google-iot-cloud)
* [Pelion Device Management](https://github.com/ARMmbed/mbed-os-example-pelion)



