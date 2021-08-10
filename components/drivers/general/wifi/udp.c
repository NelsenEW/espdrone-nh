#include "udp.h"
#include "debug_cf.h"

#define DEBUG_MODULE  "UDP"

#define UDP_SERVER_PORT         2390
#define UDP_SERVER_BUFSIZE      128

static struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
static char rx_buffer[UDP_SERVER_BUFSIZE];
static char tx_buffer[UDP_SERVER_BUFSIZE];
const int addr_family = (int)AF_INET;
const int ip_protocol = IPPROTO_IP;
static struct sockaddr_in dest_addr;
static int sock;

static xQueueHandle udpDataRx;
static xQueueHandle udpDataTx;
static UDPPacket inPacket;
static UDPPacket outPacket;

static bool isUDPInit = false;
static bool isUDPConnected = false;

static uint8_t calculate_cksum(void *data, size_t len)
{
    unsigned char *c = data;
    int i;
    unsigned char cksum = 0;

    for (i = 0; i < len; i++) {
        cksum += *(c++);
    }

    return cksum;
}

bool udpGetDataBlocking(UDPPacket *in)
{
    /* command step - receive  02  from udp rx queue */
    while (xQueueReceive(udpDataRx, in, portMAX_DELAY) != pdTRUE) {
        vTaskDelay(1);
    }; // Don't return until we get some data on the UDP

    return true;
};

bool udpSendData(uint32_t size, uint8_t *data)
{
    static UDPPacket outStage;
    outStage.size = size;
    memcpy(outStage.data, data, size);
    // Dont' block when sending
    return (xQueueSend(udpDataTx, &outStage, M2T(100)) == pdTRUE);
};

static void udp_server_rx_task(void *pvParameters)
{
    uint8_t cksum = 0;
    socklen_t socklen = sizeof(source_addr);
    
    while (true) {
        if(isUDPInit == false) {
            vTaskDelay(20);
            continue;
        }
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
        /* command step - receive  01 from Wi-Fi UDP */
        if (len < 0) {
            DEBUG_PRINT_LOCAL("recvfrom failed: errno %d", errno);
            break;
        } else if(len > UDP_RX_TX_PACKET_SIZE - 4) {
            DEBUG_PRINT_LOCAL("Received data length = %d > 64", len);
        } else {
            //copy part of the UDP packet
            rx_buffer[len] = 0;// Null-terminate whatever we received and treat like a string...
            memcpy(inPacket.data, rx_buffer, len);
            cksum = inPacket.data[len - 1];
            //remove cksum, do not belong to CRTP
            inPacket.size = len - 1;
            //check packet
            if (cksum == calculate_cksum(inPacket.data, len - 1) && inPacket.size < 64){
                xQueueSend(udpDataRx, &inPacket, M2T(2));
                if(!isUDPConnected) isUDPConnected = true;
            }else{
                DEBUG_PRINT_LOCAL("udp packet cksum unmatched");
            }

#ifdef DEBUG_UDP_RECEIVE
            static char addr_str[128];
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            DEBUG_PRINT_LOCAL("1.Received data size = %d  %02X \n cksum = %02X from %s", len, inPacket.data[0], cksum, addr_str);
            for (size_t i = 0; i < len; i++) {
                DEBUG_PRINT_LOCAL(" data[%d] = %02X ", i, inPacket.data[i]);
            }
#endif
        }
    }
}

static void udp_server_tx_task(void *pvParameters)
{
    while (TRUE) {
        if(isUDPInit == false) {
            vTaskDelay(20);
            continue;
        }
        if ((xQueueReceive(udpDataTx, &outPacket, 5) == pdTRUE) && isUDPConnected) {           
            memcpy(tx_buffer, outPacket.data, outPacket.size);       
            tx_buffer[outPacket.size] =  calculate_cksum(tx_buffer, outPacket.size);
            tx_buffer[outPacket.size + 1] = 0;
            int err = sendto(sock, tx_buffer, outPacket.size + 1, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
            if (err < 0) {
                DEBUG_PRINT_LOCAL("Error occurred during sending: errno %d", errno);
                continue;
            }
#ifdef DEBUG_UDP_SEND
            static char addr_str[128];
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
            DEBUG_PRINT_LOCAL("Send data to %s, size %d", addr_str, outPacket.size);
            for (size_t i = 0; i < outPacket.size + 1; i++) {
                DEBUG_PRINT_LOCAL(" data_send[%d] = %02X ", i, tx_buffer[i]);
            }
#endif
        }    
    }
}


esp_err_t udp_server_create(void *arg)
{   
    if (isUDPInit){
        return ESP_OK;
    }

    struct sockaddr_in *pdest_addr = &dest_addr;
    pdest_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    pdest_addr->sin_family = AF_INET;
    pdest_addr->sin_port = htons(UDP_SERVER_PORT);

    sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        DEBUG_PRINT_LOCAL("Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    DEBUG_PRINT_LOCAL("Socket created");

    int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err < 0) {
        DEBUG_PRINT_LOCAL("Socket unable to bind: errno %d", errno);
    }
    DEBUG_PRINT_LOCAL("Socket bound, port %d", UDP_SERVER_PORT);

     // This should probably be reduced to a CRTP packet size
    udpDataRx = xQueueCreate(5, sizeof(UDPPacket)); /* Buffer packets (max 64 bytes) */
    DEBUG_QUEUE_MONITOR_REGISTER(udpDataRx);
    udpDataTx = xQueueCreate(5, sizeof(UDPPacket)); /* Buffer packets (max 64 bytes) */
    DEBUG_QUEUE_MONITOR_REGISTER(udpDataTx);

    xTaskCreate(udp_server_tx_task, UDP_TX_TASK_NAME, UDP_TX_TASK_STACKSIZE, NULL, UDP_TX_TASK_PRI, NULL);
    xTaskCreate(udp_server_rx_task, UDP_RX_TASK_NAME, UDP_RX_TASK_STACKSIZE, NULL, UDP_RX_TASK_PRI, NULL);
    isUDPInit = true;
    return ESP_OK;
}
