/*
 * FreeRTOS V202411.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/*
 * Demo for showing use of the managed MQTT API.
 *
 * The example shown below uses this API to create MQTT messages and
 * send them over the connection established using FreeRTOS sockets.
 * The example is single threaded and uses statically allocated memory;
 *
 * !!! NOTE !!!
 * This MQTT demo does not authenticate the server nor the client.
 * Hence, this demo should not be used as production ready code.
 *
 * Also see https://www.freertos.org/mqtt/mqtt-agent-demo.html? for an
 * alternative run time model whereby coreMQTT runs in an autonomous
 * background agent task.  Executing the MQTT protocol in an agent task
 * removes the need for the application writer to explicitly manage any MQTT
 * state or call the MQTT_ProcessLoop() API function. Using an agent task
 * also enables multiple application tasks to more easily share a single
 * MQTT connection.
 */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* MQTT library includes. */
#include "core_mqtt.h"

/* Transport interface implementation include header for esp8266AT connection. */
#include "transport_esp8266.h"

#include "app_data_types.h"
#include "mqtt_task.h"
#include "drivers/digital_io.h"

#define mLED                                     mLED_MQTT

#define configASSERT(x) \
if (!(x)) { \
  for (;;) { \
    digitalIOToggle(mERROR_LED); \
    vTaskDelay(pdMS_TO_TICKS(300)); \
    } \
}

/**
 * @brief The MQTT client identifier used in this example.  Each client identifier
 * must be unique so edit as required to ensure no two clients connecting to the
 * same broker use the same client identifier.
 *
 *!!! Please note a #defined constant is used for convenience of demonstration
 *!!! only.  Production devices can use something unique to the device that can
 *!!! be read by software, such as a production serial number, instead of a
 *!!! hard coded constant.
 *
 * #define democonfigCLIENT_IDENTIFIER				"insert here."
 */
#define democonfigCLIENT_IDENTIFIER              "UNO_R3"


/**
 * @brief MQTT broker end point to connect to.
 *
 * @note If you would like to setup an MQTT broker for running this demo,
 * please see `mqtt_broker_setup.txt`.
 *
 * #define democonfigMQTT_BROKER_ENDPOINT				"insert here."
 */
#define democonfigMQTT_BROKER_ENDPOINT           "192.168.0.235"


/**
 * @brief The port to use for the demo.
 *
 * #define democonfigMQTT_BROKER_PORT					( insert here. )
 */
#define democonfigMQTT_BROKER_PORT               "1883"


/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define democonfigNETWORK_BUFFER_SIZE    ( 32U )

/*-----------------------------------------------------------*/

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define mqttexampleRETRY_MAX_ATTEMPTS                     ( 3U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define mqttexampleRETRY_MAX_BACKOFF_DELAY_MS             ( 5000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define mqttexampleRETRY_BACKOFF_BASE_MS                  ( 500U )

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define mqttexampleCONNACK_RECV_TIMEOUT_MS                ( 1000U )

/**
 * @brief The prefix to the topic(s) subscribe(d) to and publish(ed) to in the example.
 *
 * The topic name starts with the client identifier to ensure that each demo
 * interacts with a unique topic name.
 */
#define mqttexampleRX_TOPIC_NAME                           "/home/garage/control"
#define mqttexampleTX_TOPIC_NAME                           "/home/garage/state"

/**
 * @brief The number of topic filters to subscribe.
 */
#define mqttexampleTOPIC_COUNT                            ( 1 )

/**
 * @brief The size of the buffer for each topic string.
 */
#define mqttexampleTOPIC_BUFFER_SIZE                      ( 32U )

/**
 * @brief The MQTT message published in this example.
 */
#define mqttexampleMESSAGE                                "Hello World from UNO R3"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define mqttexampleDELAY_BETWEEN_DEMO_ITERATIONS_TICKS    ( pdMS_TO_TICKS( 3000U ) )

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 * Refer to FreeRTOS-Plus/Demo/coreMQTT_Windows_Simulator/readme.txt for more details.
 */
#define mqttexamplePROCESS_LOOP_TIMEOUT_MS                ( 1000U )

/**
 * @brief The keep-alive timeout period reported to the broker while establishing
 * an MQTT connection.
 *
 * It is the responsibility of the client to ensure that the interval between
 * control packets being sent does not exceed this keep-alive value. In the
 * absence of sending any other control packets, the client MUST send a
 * PINGREQ packet.
 */
#define mqttexampleKEEP_ALIVE_TIMEOUT_SECONDS             ( 60U )

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define mqttexampleDELAY_BETWEEN_PUBLISHES_TICKS          ( pdMS_TO_TICKS( 2000U ) )

/**
 * @brief The length of the outgoing publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for outgoing publishes.
 * Number of publishes = ulMaxPublishCount * mqttexampleTOPIC_COUNT
 * Update in ulMaxPublishCount needs updating mqttexampleOUTGOING_PUBLISH_RECORD_LEN.
 */
#define mqttexampleOUTGOING_PUBLISH_RECORD_LEN            ( 10U )

/**
 * @brief The length of the incoming publish records array used by the coreMQTT
 * library to track QoS > 0 packet ACKS for incoming publishes.
 * Number of publishes = ulMaxPublishCount * mqttexampleTOPIC_COUNT
 * Update in ulMaxPublishCount needs updating mqttexampleINCOMING_PUBLISH_RECORD_LEN.
 */
#define mqttexampleINCOMING_PUBLISH_RECORD_LEN            ( 10U )

/**
 * @brief Milliseconds per second.
 */
#define MILLISECONDS_PER_SECOND                           ( 1000U )

/**
 * @brief Milliseconds per FreeRTOS tick.
 */
#define MILLISECONDS_PER_TICK                             ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )

/*-----------------------------------------------------------*/

/**
 * @brief Each compilation unit that consumes the NetworkContext must define it.
 * It should contain a single pointer to the type of your desired transport.
 * When using multiple transports in the same compilation unit, define this pointer as void *.
 *
 * @note Transport stacks are defined in FreeRTOS-Plus/Source/Application-Protocols/network_transport.
 */
/* NOT USED */
/*
struct NetworkContext
{
    PlaintextTransportParams_t * pParams;
};
*/
/*-----------------------------------------------------------*/

/**
 * @brief Sends an MQTT Connect packet over the already connected TLS over TCP connection.
 *
 * @param[in, out] pxMQTTContext MQTT context pointer.
 * @param[in] xNetworkContext network context.
 */
static void prvCreateMQTTConnectionWithBroker( MQTTContext_t * pxMQTTContext,
                                               NetworkContext_t * pxNetworkContext );

/**
 * @brief Function to update variable #Context with status
 * information from Subscribe ACK. Called by the event callback after processing
 * an incoming SUBACK packet.
 *
 * @param[in] Server response to the subscription request.
 */
static void prvUpdateSubAckStatus( MQTTPacketInfo_t * pxPacketInfo );

/**
 * @brief Subscribes to the topic as specified in mqttexampleTOPIC at the top of
 * this file. In the case of a Subscribe ACK failure, then subscription is
 * retried using an exponential backoff strategy with jitter.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 */
static void prvMQTTSubscribeWithBackoffRetries( MQTTContext_t * pxMQTTContext );

/**
 * @brief Publishes a message mqttexampleMESSAGE on mqttexampleTOPIC topic.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 */
static void prvMQTTPublishToTopics( MQTTContext_t *pxMQTTContext );

/**
 * @brief Unsubscribes from the previously subscribed topic as specified
 * in mqttexampleTOPIC.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 */
static void prvMQTTUnsubscribeFromTopics( MQTTContext_t * pxMQTTContext );

/**
 * @brief The timer query function provided to the MQTT context.
 *
 * @return Time in milliseconds.
 */
static uint32_t prvGetTimeMs( void );

/**
 * @brief Process a response or ack to an MQTT request (PING, PUBLISH,
 * SUBSCRIBE or UNSUBSCRIBE). This function processes PINGRESP, PUBACK,
 * PUBREC, PUBREL, PUBCOMP, SUBACK, and UNSUBACK.
 *
 * @param[in] pxIncomingPacket is a pointer to structure containing deserialized
 * MQTT response.
 * @param[in] usPacketId is the packet identifier from the ack received.
 */
static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId );

/**
 * @brief Process incoming Publish message.
 *
 * @param[in] pxPublishInfo is a pointer to structure containing deserialized
 * Publish message.
 */
static void prvMQTTProcessIncomingPublish( MQTTContext_t * pxMQTTContext, MQTTPublishInfo_t * pxPublishInfo );

/**
 * @brief The application callback function for getting the incoming publishes,
 * incoming acks, and ping responses reported from the MQTT library.
 *
 * @param[in] pxMQTTContext MQTT context pointer.
 * @param[in] pxPacketInfo Packet Info pointer for the incoming packet.
 * @param[in] pxDeserializedInfo Deserialized information from the incoming packet.
 */
static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo );

/**
 * @brief Call #MQTT_ProcessLoop in a loop for the duration of a timeout or
 * #MQTT_ProcessLoop returns a failure.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in] ulTimeoutMs Duration to call #MQTT_ProcessLoop for.
 *
 * @return Returns the return value of the last call to #MQTT_ProcessLoop.
 */
static MQTTStatus_t prvProcessLoopWithTimeout( MQTTContext_t * pMqttContext,
                                               uint32_t ulTimeoutMs );

/**
 * @brief Initialize the topic filter string and SUBACK buffers.
 */
static void prvInitializeTopicBuffers( void );

/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucSharedBuffer[ democonfigNETWORK_BUFFER_SIZE ];

/**
 * @brief Global entry time into the application to use as a reference timestamp
 * in the #prvGetTimeMs function. #prvGetTimeMs will always return the difference
 * between the current time and the global entry time. This will reduce the chances
 * of overflow for the 32 bit unsigned integer used for holding the timestamp.
 */
static uint32_t ulGlobalEntryTimeMs = 0;

/**
 * @brief Packet Identifier generated when Publish request was sent to the broker;
 * it is used to match received Publish ACK to the transmitted Publish packet.
 */
static uint16_t usPublishPacketIdentifier;

/**
 * @brief Packet Identifier generated when Subscribe request was sent to the broker;
 * it is used to match received Subscribe ACK to the transmitted Subscribe packet.
 */
static uint16_t usSubscribePacketIdentifier;

/**
 * @brief Packet Identifier generated when Unsubscribe request was sent to the broker;
 * it is used to match received Unsubscribe response to the transmitted Unsubscribe
 * request.
 */
static uint16_t usUnsubscribePacketIdentifier;

/**
 * @brief A pair containing a topic filter and its SUBACK status.
 */
typedef struct topicFilterContext
{
    uint8_t pcTopicFilter[ mqttexampleTOPIC_BUFFER_SIZE ];
    MQTTSubAckStatus_t xSubAckStatus;
} topicFilterContext_t;

/**
 * @brief An array containing the context of a SUBACK; the SUBACK status
 * of a filter is updated when the event callback processes a SUBACK.
 */
static topicFilterContext_t xTopicFilterContext[ mqttexampleTOPIC_COUNT ];


/** @brief Static buffer used to hold MQTT messages being sent and received. */
static MQTTFixedBuffer_t xBuffer =
{
    ucSharedBuffer,
    democonfigNETWORK_BUFFER_SIZE
};

/**
 * @brief Array to track the outgoing publish records for outgoing publishes
 * with QoS > 0.
 *
 * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
 *
 */
static MQTTPubAckInfo_t pOutgoingPublishRecords[ mqttexampleOUTGOING_PUBLISH_RECORD_LEN ];

/**
 * @brief Array to track the incoming publish records for incoming publishes
 * with QoS > 0.
 *
 * This is passed into #MQTT_InitStatefulQoS to allow for QoS > 0.
 *
 */
static MQTTPubAckInfo_t pIncomingPublishRecords[ mqttexampleINCOMING_PUBLISH_RECORD_LEN ];

/*-----------------------------------------------------------*/

static app_data_handle_t *app_data;

/*
 * @brief The Example shown below uses MQTT APIs to create MQTT messages and
 * send them over the server-authenticated network connection established with the
 * MQTT broker. This example is single-threaded and uses statically allocated
 * memory. It uses QoS2 for sending and receiving messages from the broker.
 *
 * This MQTT client subscribes to the topic as specified in mqttexampleTOPIC at the
 * top of this file by sending a subscribe packet and waiting for a subscribe
 * acknowledgment (SUBACK) from the broker. The client will then publish to the
 * same topic it subscribed to, therefore expecting that all outgoing messages will be
 * sent back from the broker.
 */
void MQTTtask( void * pvParameters )
{
    MQTTContext_t xMQTTContext = { 0 };
    esp8266TransportStatus_t xNetworkStatus;

    app_data = (app_data_handle_t*) pvParameters;

    /* Set the entry time of the demo application. This entry time will be used
     * to calculate relative time elapsed in the execution of the demo application,
     * by the timer utility function that is provided to the MQTT library.
     */
    ulGlobalEntryTimeMs = prvGetTimeMs();

    /**************************** Initialize. *****************************/
    prvInitializeTopicBuffers();

    /****************************** Connect. ******************************/
    xNetworkStatus = esp8266AT_Connect(democonfigMQTT_BROKER_ENDPOINT, democonfigMQTT_BROKER_PORT);
    configASSERT(xNetworkStatus == ESP8266_TRANSPORT_SUCCESS);

    /* Send an MQTT CONNECT packet over the established TLS connection,
     * and wait for the connection acknowledgment (CONNACK) packet. */
    prvCreateMQTTConnectionWithBroker( &xMQTTContext, NULL );

    /**************************** Subscribe. ******************************/
    /* If the server rejected the subscription request, attempt to resubscribe to the
     * topic. Attempts are made according to the exponential backoff retry strategy
     * implemented in BackoffAlgorithm. */
    prvMQTTSubscribeWithBackoffRetries( &xMQTTContext );

    for( ; ; )
    {

#ifdef  DEBUG_LED
        digitalIOToggle(mLED);
#endif

        prvProcessLoopWithTimeout(&xMQTTContext, mqttexamplePROCESS_LOOP_TIMEOUT_MS); 
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
/*-----------------------------------------------------------*/

static void prvCreateMQTTConnectionWithBroker( MQTTContext_t * pxMQTTContext,
                                               NetworkContext_t * pxNetworkContext )
{
    MQTTStatus_t xResult;
    MQTTConnectInfo_t xConnectInfo;
    bool xSessionPresent;
    TransportInterface_t xTransport;

    /***
     * For readability, error handling in this function is restricted to the use of
     * asserts().
     ***/

    /* Fill in Transport Interface send and receive function pointers. */
    xTransport.pNetworkContext = pxNetworkContext;
    xTransport.send = esp8266AT_send;
    xTransport.recv = esp8266AT_recv;
    xTransport.writev = NULL;

    /* Initialize MQTT library. */
    xResult = MQTT_Init( pxMQTTContext, &xTransport, prvGetTimeMs, prvEventCallback, &xBuffer );
    configASSERT( xResult == MQTTSuccess );
    xResult = MQTT_InitStatefulQoS( pxMQTTContext,
                                    pOutgoingPublishRecords,
                                    mqttexampleOUTGOING_PUBLISH_RECORD_LEN,
                                    pIncomingPublishRecords,
                                    mqttexampleINCOMING_PUBLISH_RECORD_LEN );
    configASSERT( xResult == MQTTSuccess );

    /* Some fields are not used in this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xConnectInfo, 0x00, sizeof( xConnectInfo ) );

    /* Start with a clean session i.e. direct the MQTT broker to discard any
     * previous session data. Also, establishing a connection with clean session
     * will ensure that the broker does not store any data when this client
     * gets disconnected. */
    xConnectInfo.cleanSession = true;

    /* The client identifier is used to uniquely identify this MQTT client to
     * the MQTT broker. In a production device the identifier can be something
     * unique, such as a device serial number. */
    xConnectInfo.pClientIdentifier = democonfigCLIENT_IDENTIFIER;
    xConnectInfo.clientIdentifierLength = ( uint16_t ) strlen( democonfigCLIENT_IDENTIFIER );

    /* Set MQTT keep-alive period. If the application does not send packets at an interval less than
     * the keep-alive period, the MQTT library will send PINGREQ packets. */
    xConnectInfo.keepAliveSeconds = mqttexampleKEEP_ALIVE_TIMEOUT_SECONDS;

    /* Send MQTT CONNECT packet to broker. LWT is not used in this demo, so it
     * is passed as NULL. */
    xResult = MQTT_Connect( pxMQTTContext,
                            &xConnectInfo,
                            NULL,
                            mqttexampleCONNACK_RECV_TIMEOUT_MS,
                            &xSessionPresent );
    configASSERT( xResult == MQTTSuccess );

    /* Successfully established and MQTT connection with the broker. */
}
/*-----------------------------------------------------------*/

static void prvUpdateSubAckStatus( MQTTPacketInfo_t * pxPacketInfo )
{
    MQTTStatus_t xResult = MQTTSuccess;
    uint8_t * pucPayload = NULL;
    size_t ulSize = 0;
    uint32_t ulTopicCount = 0U;

    xResult = MQTT_GetSubAckStatusCodes( pxPacketInfo, &pucPayload, &ulSize );

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    configASSERT( xResult == MQTTSuccess );

    for( ulTopicCount = 0; ulTopicCount < ulSize; ulTopicCount++ )
    {
        xTopicFilterContext[ ulTopicCount ].xSubAckStatus = pucPayload[ ulTopicCount ];
    }
}
/*-----------------------------------------------------------*/

static void prvMQTTSubscribeWithBackoffRetries( MQTTContext_t * pxMQTTContext )
{
    MQTTStatus_t xResult = MQTTSuccess;
    uint8_t counter = mqttexampleRETRY_MAX_ATTEMPTS;
    uint16_t usNextRetryBackOff = mqttexampleRETRY_BACKOFF_BASE_MS;
    MQTTSubscribeInfo_t xMQTTSubscription[ mqttexampleTOPIC_COUNT ];
    bool xFailedSubscribeToTopic = false;
    uint32_t ulTopicCount = 0U;

    /* Some fields not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTSubscription, 0x00, sizeof( xMQTTSubscription ) );

    /* Get a unique packet id. */
    usSubscribePacketIdentifier = MQTT_GetPacketId( pxMQTTContext );

    /* Populate subscription list. */
    for( ulTopicCount = 0; ulTopicCount < mqttexampleTOPIC_COUNT; ulTopicCount++ )
    {
        xMQTTSubscription[ ulTopicCount ].qos = MQTTQoS2;
        xMQTTSubscription[ ulTopicCount ].pTopicFilter = xTopicFilterContext[ ulTopicCount ].pcTopicFilter;
        xMQTTSubscription[ ulTopicCount ].topicFilterLength = ( uint16_t ) strlen( xTopicFilterContext[ ulTopicCount ].pcTopicFilter );
    }

    do
    {
        /* The client is now connected to the broker. Subscribe to the topic
         * as specified in mqttexampleTOPIC at the top of this file by sending a
         * subscribe packet then waiting for a subscribe acknowledgment (SUBACK).
         * This client will then publish to the same topic it subscribed to, so it
         * will expect all the messages it sends to the broker to be sent back to it
         * from the broker. This demo uses QOS2 in Subscribe, therefore, the Publish
         * messages received from the broker will have QOS2. */
        xResult = MQTT_Subscribe( pxMQTTContext,
                                  xMQTTSubscription,
                                  sizeof( xMQTTSubscription ) / sizeof( MQTTSubscribeInfo_t ),
                                  usSubscribePacketIdentifier );
        configASSERT( xResult == MQTTSuccess );

        /* Process incoming packet from the broker. After sending the subscribe, the
         * client may receive a publish before it receives a subscribe ack. Therefore,
         * call generic incoming packet processing function. Since this demo is
         * subscribing to the topic to which no one is publishing, probability of
         * receiving Publish message before subscribe ack is zero; but application
         * must be ready to receive any packet.  This demo uses the generic packet
         * processing function everywhere to highlight this fact. */
        xResult = prvProcessLoopWithTimeout( pxMQTTContext, mqttexamplePROCESS_LOOP_TIMEOUT_MS );
        configASSERT( xResult == MQTTSuccess );

        /* Reset flag before checking suback responses. */
        xFailedSubscribeToTopic = false;

        /* Check if recent subscription request has been rejected. #xTopicFilterContext is updated
         * in the event callback to reflect the status of the SUBACK sent by the broker. It represents
         * either the QoS level granted by the server upon subscription, or acknowledgement of
         * server rejection of the subscription request. */
        for( ulTopicCount = 0; ulTopicCount < mqttexampleTOPIC_COUNT; ulTopicCount++ )
        {
            if( xTopicFilterContext[ ulTopicCount ].xSubAckStatus == MQTTSubAckFailure )
            {
                xFailedSubscribeToTopic = true;

                /* Generate a random number and calculate backoff value (in milliseconds) for
                 * the next connection retry.
                 * Note: It is recommended to seed the random number generator with a device-specific
                 * entropy source so that possibility of multiple devices retrying failed network operations
                 * at similar intervals can be avoided. */

                counter--;
                if(counter)
                {
                    /* Backoff before the next re-subscribe attempt. */
                    vTaskDelay( pdMS_TO_TICKS( usNextRetryBackOff ) );
                }

                break;
            }
        }
    } while( ( xFailedSubscribeToTopic == true ) && ( counter ) );
}
/*-----------------------------------------------------------*/

static void prvMQTTPublishToTopics( MQTTContext_t *pxMQTTContext )
{
    MQTTStatus_t xResult;
    MQTTPublishInfo_t xMQTTPublishInfo;
    //char msg[5];

    /***
     * For readability, error handling in this function is restricted to the use of
     * asserts().
     ***/

    //snprintf(msg, 5, "%u", data);

    /* Some fields are not used by this demo so start with everything at 0. */
    ( void ) memset( ( void * ) &xMQTTPublishInfo, 0x00, sizeof( xMQTTPublishInfo ) );

    /* This demo uses QoS2 */
    xMQTTPublishInfo.qos = MQTTQoS2;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = mqttexampleTX_TOPIC_NAME;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) strlen( xMQTTPublishInfo.pTopicName );
    xMQTTPublishInfo.pPayload = &(app_data->sensor_read);
    xMQTTPublishInfo.payloadLength = sizeof(hcsr04_data_t);

    /* Get a unique packet id. */
    usPublishPacketIdentifier = MQTT_GetPacketId( pxMQTTContext );

    /* Send PUBLISH packet. */
    xResult = MQTT_Publish( pxMQTTContext, &xMQTTPublishInfo, usPublishPacketIdentifier );
    configASSERT( xResult == MQTTSuccess );
}
/*-----------------------------------------------------------*/

#if 0
static void prvMQTTUnsubscribeFromTopics( MQTTContext_t * pxMQTTContext )
{
    MQTTStatus_t xResult;
    MQTTSubscribeInfo_t xMQTTSubscription[ mqttexampleTOPIC_COUNT ];
    uint32_t ulTopicCount;

    /* Some fields are not used by this demo so start with everything at 0. */
    memset( ( void * ) &xMQTTSubscription, 0x00, sizeof( xMQTTSubscription ) );

    /* Populate subscription list. */
    for( ulTopicCount = 0; ulTopicCount < mqttexampleTOPIC_COUNT; ulTopicCount++ )
    {
        xMQTTSubscription[ ulTopicCount ].qos = MQTTQoS2;
        xMQTTSubscription[ ulTopicCount ].pTopicFilter = xTopicFilterContext[ ulTopicCount ].pcTopicFilter;
        xMQTTSubscription[ ulTopicCount ].topicFilterLength = ( uint16_t ) strlen( xTopicFilterContext[ ulTopicCount ].pcTopicFilter );

        //LogInfo( ( "Unsubscribing from topic %s.\r\n", xTopicFilterContext[ ulTopicCount ].pcTopicFilter ) );
    }

    /* Get next unique packet identifier. */
    usUnsubscribePacketIdentifier = MQTT_GetPacketId( pxMQTTContext );
    /* Make sure the packet id obtained is valid. */
    configASSERT( usUnsubscribePacketIdentifier != 0 );

    /* Send UNSUBSCRIBE packet. */
    xResult = MQTT_Unsubscribe( pxMQTTContext,
                                xMQTTSubscription,
                                sizeof( xMQTTSubscription ) / sizeof( MQTTSubscribeInfo_t ),
                                usUnsubscribePacketIdentifier );

    configASSERT( xResult == MQTTSuccess );
}
/*-----------------------------------------------------------*/
#endif

static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    switch( pxIncomingPacket->type )
    {
        case MQTT_PACKET_TYPE_PUBACK:
            break;

        case MQTT_PACKET_TYPE_SUBACK:

            /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
             * It contains the status code indicating server approval/rejection for the subscription to the single topic
             * requested. The SUBACK will be parsed to obtain the status code, and this status code will be stored in global
             * variable #xTopicFilterContext. */
            prvUpdateSubAckStatus( pxIncomingPacket );

            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usSubscribePacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_UNSUBACK:
            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usUnsubscribePacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_PINGRESP:

            /* Nothing to be done from application as library handles
             * PINGRESP with the use of MQTT_ProcessLoop API function. */
            break;

        case MQTT_PACKET_TYPE_PUBREC:
            break;

        case MQTT_PACKET_TYPE_PUBREL:

            /* Nothing to be done from application as library handles
             * PUBREL. */
            break;

        case MQTT_PACKET_TYPE_PUBCOMP:

            /* Nothing to be done from application as library handles
             * PUBCOMP. */
            break;

        /* Any other packet type is invalid. */
    }
}

/*-----------------------------------------------------------*/

static void prvMQTTProcessIncomingPublish( MQTTContext_t * pxMQTTContext, MQTTPublishInfo_t * pxPublishInfo )
{
    configASSERT( pxPublishInfo != NULL );

    /* Verify the received publish is for one of the topics that's been subscribed to. */
    if( ( pxPublishInfo->topicNameLength == strlen( xTopicFilterContext[0].pcTopicFilter ) ) &&
        ( strncmp( xTopicFilterContext[0].pcTopicFilter, pxPublishInfo->pTopicName, pxPublishInfo->topicNameLength ) == 0 ) )
    {
        /* Verify the message received matches expected commands */

        if( strncmp( "ON", ( const char * ) ( pxPublishInfo->pPayload ), pxPublishInfo->payloadLength ) == 0 )
        {
            digitalIOSet(mLED_0, pdTRUE);
        }

        else if( strncmp( "OFF", ( const char * ) ( pxPublishInfo->pPayload ), pxPublishInfo->payloadLength ) == 0 )
        {
            digitalIOSet(mLED_0, pdFALSE);
        }

        else if( strncmp( "UPDATE", ( const char * ) ( pxPublishInfo->pPayload ), pxPublishInfo->payloadLength ) == 0 )
        {
            /* Activate sensor task to get a new read */
            vTaskResume(app_data->sensor_task);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
            prvMQTTPublishToTopics( pxMQTTContext );
        }
    }
}

/*-----------------------------------------------------------*/

static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* The MQTT context is not used in this function. */
    ( void ) pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( pxMQTTContext, pxDeserializedInfo->pPublishInfo );
    }
    else
    {
        prvMQTTProcessResponse( pxPacketInfo, pxDeserializedInfo->packetIdentifier );
    }
}

/*-----------------------------------------------------------*/

static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * MILLISECONDS_PER_TICK;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTimeMs = ( uint32_t ) ( ulTimeMs - ulGlobalEntryTimeMs );

    return ulTimeMs;
}

/*-----------------------------------------------------------*/

static MQTTStatus_t prvProcessLoopWithTimeout( MQTTContext_t * pMqttContext,
                                               uint32_t ulTimeoutMs )
{
    uint32_t ulMqttProcessLoopTimeoutTime;
    uint32_t ulCurrentTime;

    MQTTStatus_t eMqttStatus = MQTTSuccess;

    ulCurrentTime = pMqttContext->getTime();
    if (ulCurrentTime + ulTimeoutMs < ulCurrentTime) //overflow
        ulMqttProcessLoopTimeoutTime = ulTimeoutMs;
    else
        ulMqttProcessLoopTimeoutTime = ulCurrentTime + ulTimeoutMs;

    /* Call MQTT_ProcessLoop multiple times a timeout happens, or
     * MQTT_ProcessLoop fails. */
    while( ( ulCurrentTime < ulMqttProcessLoopTimeoutTime ) || (eMqttStatus == MQTTNeedMoreBytes) )
    {
        eMqttStatus = MQTT_ProcessLoop( pMqttContext );
        ulTimeoutMs = pMqttContext->getTime();
        if (ulCurrentTime >= ulTimeoutMs) //overflow
            break;
        ulCurrentTime = ulTimeoutMs;
    }

    return eMqttStatus;
}

/*-----------------------------------------------------------*/

static void prvInitializeTopicBuffers( void )
{
    uint32_t ulTopicCount;
    int xCharactersWritten;


    for( ulTopicCount = 0; ulTopicCount < mqttexampleTOPIC_COUNT; ulTopicCount++ )
    {
        /* Write topic strings into buffers. */
        xCharactersWritten = snprintf( xTopicFilterContext[ ulTopicCount ].pcTopicFilter,
                                       mqttexampleTOPIC_BUFFER_SIZE,
                                       "%s", mqttexampleRX_TOPIC_NAME);

        configASSERT( xCharactersWritten >= 0 && xCharactersWritten < mqttexampleTOPIC_BUFFER_SIZE );

        /* Assign topic string to its corresponding SUBACK code initialized as a failure. */
        xTopicFilterContext[ ulTopicCount ].xSubAckStatus = MQTTSubAckFailure;
    }
}

/*-----------------------------------------------------------*/
