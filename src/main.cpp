#include <MqttClient.h>
#include <igris/time/systime.h>

#define MQTT_ID "TEST-ID"
const char *MQTT_TOPIC_SUB = "test/" MQTT_ID "/sub";
const char *MQTT_TOPIC_PUB = "test/" MQTT_ID "/pub";
MqttClient *mqtt = NULL;

#define LOG_PRINTFLN(fmt, ...)
// printfln_P(PSTR(fmt), ##__VA_ARGS__)
#define LOG_SIZE_MAX 128
void printfln_P(const char *fmt, ...)
{
    // char buf[LOG_SIZE_MAX];
    // va_list ap;
    // va_start(ap, fmt);
    // vsnprintf_P(buf, LOG_SIZE_MAX, fmt, ap);
    // va_end(ap);
    // Serial.println(buf);
}

class System : public MqttClient::System
{
public:
    unsigned long millis() const
    {
        return igris::millis();
    }
};

class Network
{
public:
    Network() {}

    int connect(const char *hostname, int port)
    {
        // TCP connection is already established otherwise do it here
        return 0;
    }

    int read(unsigned char *buffer, int len, unsigned long timeoutMs)
    {
        return 0;
    }

    int write(unsigned char *buffer, int len, unsigned long timeoutMs)
    {
        return 0;
    }

    int disconnect()
    {
        return 0;
    }

private:
} *network = NULL;

void setup()
{
    // Setup network
    network = new Network;
    // Setup MqttClient
    MqttClient::System *mqttSystem = new System;
    //    MqttClient::Logger *mqttLogger =
    //        new MqttClient::LoggerImpl<HardwareSerial>(Serial);
    MqttClient::Network *mqttNetwork =
        new MqttClient::NetworkImpl<Network>(*network, *mqttSystem);
    //// Make 128 bytes send buffer
    MqttClient::Buffer *mqttSendBuffer = new MqttClient::ArrayBuffer<128>();
    //// Make 128 bytes receive buffer
    MqttClient::Buffer *mqttRecvBuffer = new MqttClient::ArrayBuffer<128>();
    //// Allow up to 2 subscriptions simultaneously
    MqttClient::MessageHandlers *mqttMessageHandlers =
        new MqttClient::MessageHandlersImpl<2>();
    //// Configure client options
    MqttClient::Options mqttOptions;
    ////// Set command timeout to 10 seconds
    mqttOptions.commandTimeoutMs = 10000;
    //// Make client object
    mqtt = new MqttClient(mqttOptions,
                          //*mqttLogger,
                          *(MqttClient::Logger *)nullptr,
                          *mqttSystem,
                          *mqttNetwork,
                          *mqttSendBuffer,
                          *mqttRecvBuffer,
                          *mqttMessageHandlers);
}

// ============== Subscription callback ========================================
void processMessage(MqttClient::MessageData &md)
{
    const MqttClient::Message &msg = md.message;
    char payload[msg.payloadLen + 1];
    memcpy(payload, msg.payload, msg.payloadLen);
    payload[msg.payloadLen] = '\0';
    LOG_PRINTFLN("Message arrived: qos %d, retained %d, dup %d, packetid %d, "
                 "payload:[%s]",
                 msg.qos,
                 msg.retained,
                 msg.dup,
                 msg.id,
                 payload);
}

// ============== Main loop ====================================================
void loop()
{
    // Check connection status
    if (!mqtt->isConnected())
    {
        // Re-establish TCP connection with MQTT broker
        network->disconnect();
        network->connect("mymqttserver.com", 1883);
        // Start new MQTT connection
        LOG_PRINTFLN("Connecting");
        MqttClient::ConnectResult connectResult;
        // Connect
        {
            MQTTPacket_connectData options = MQTTPacket_connectData_initializer;
            options.MQTTVersion = 4;
            options.clientID.cstring = (char *)MQTT_ID;
            options.cleansession = true;
            options.keepAliveInterval = 15; // 15 seconds
            MqttClient::Error::type rc = mqtt->connect(options, connectResult);
            if (rc != MqttClient::Error::SUCCESS)
            {
                LOG_PRINTFLN("Connection error: %i", rc);
                return;
            }
        }
        // Subscribe
        {
            MqttClient::Error::type rc = mqtt->subscribe(
                MQTT_TOPIC_SUB, MqttClient::QOS0, processMessage);
            if (rc != MqttClient::Error::SUCCESS)
            {
                LOG_PRINTFLN("Subscribe error: %i", rc);
                LOG_PRINTFLN("Drop connection");
                mqtt->disconnect();
                return;
            }
        }
    }
    else
    {
        // Publish
        {
            const char *buf = "Hello";
            MqttClient::Message message;
            message.qos = MqttClient::QOS0;
            message.retained = false;
            message.dup = false;
            message.payload = (void *)buf;
            message.payloadLen = strlen(buf);
            mqtt->publish(MQTT_TOPIC_PUB, message);
        }
        // Idle for 30 seconds
        mqtt->yield(30000L);
    }
}

int main()
{
    setup();
    while (1)
    {
        loop();
    }
}