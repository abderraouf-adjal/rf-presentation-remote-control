/* SPDX-License-Identifier: ISC */
/**
 * Licensing and copyright: See the file `LICENSE.txt`
 */

/**
 * Project name: Wireless Presentation Remote Control - The slave/receiver
 * Project version: v0.9.0 / 2021-02-14
 * License: ISC License.
 * Author information: Abderraouf Adjal
 * Author email: abderraouf.adjal@gmail.com
 *
 * Requirements
 * # Hardware:
 *   - ESP8266EX (NodeMCU, D1 mini, D1 mini Pro):
 *         -- https://www.espressif.com/en/products/socs/esp8266
 *         -- https://www.elecrow.com/download/ESP-12F.pdf
 * # Software:
 *   - ESP8266 Arduino Core (Framework), tested with v2.7.4 (https://github.com/esp8266/Arduino)
 * # NOTES:
 *   - IO Configs:     Built-in LED: GPIO-2/D4.
 *                     RF Signal UART TX: GPIO-2/D4 (HW UART2/Serial1).
 */


//#define SYS_IN_DEBUG

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>


volatile uint8_t espnow_payload = 0;
volatile uint8_t espnow_payload_counter = 0;
volatile uint8_t send_payload_flag = 0;

#define LED_PIN 2 /* D4 */
//#define ESPNOW_CHANNEL 14

// master/sender STA MAC (D1 Mini): 3E:71:BF:26:6D:00
uint8_t masterMac[] = { 0x3E, 0x71, 0xBF, 0x26, 0x6D, 0x00 }; // Replace with the AP MAC address of the slave/receiver
// thisMac (our) (NodeMCU): AP MAC: 86:F3:EB:1A:C5:01
uint8_t thisMac[] = { 0x86, 0xF3, 0xEB, 0x1A, 0xC5, 0x01 }; // Replace with the AP MAC address of the slave/receiver

void OnDataRecv(uint8_t* mac_addr, uint8_t* data, uint8_t len);

// Init ESP Now with fallback
void InitESPNow()
{
    if (esp_now_init() != 0) {
        ESP.restart();
    }

    //esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
}

// callback when data is recv from Master
void ICACHE_FLASH_ATTR OnDataRecv(uint8_t* mac_addr, uint8_t* data, uint8_t len)
{
    // If not current UART sending
    if ((!send_payload_flag)
        && (len == 2)
        && (! memcmp(mac_addr, masterMac, sizeof(masterMac)))
    ) {
        if (data[1] != espnow_payload_counter) {
            digitalWrite(LED_PIN, LOW);
            espnow_payload = data[0];
            espnow_payload_counter = data[1];
            send_payload_flag = 0xFF;
        }
    }

#ifdef SYS_IN_DEBUG
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Recv from: ");
    Serial.print(macStr);
    Serial.print("  |  Recv Data: ");
    Serial.print(data[0]);
    Serial.print("  ");
    Serial.println(data[1]);
#endif
}

void initVariant(void) {
    //wifi_set_macaddr(SOFTAP_IF, &thisMac[0]);
    wifi_set_macaddr(STATION_IF, &thisMac[0]);
}

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    Serial.begin(115200);
#ifndef SYS_IN_DEBUG
    Serial.flush();
    Serial.end();
#endif

    //Set device in AP mode to begin with
    //WiFi.mode(WIFI_AP_STA);
    //WiFi.mode(WIFI_AP);
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    //WiFi.setSleepMode(WIFI_MODEM_SLEEP);
    
#ifdef SYS_IN_DEBUG
    // This is the mac address of the Slave in AP Mode
    //Serial.print("AP MAC: ");
    //Serial.println(WiFi.softAPmacAddress());

    // This is the mac address of the Master in Station Mode
    Serial.print("STA MAC: ");
    Serial.println(WiFi.macAddress());
#endif

    // Init ESPNow with a fallback logic
    InitESPNow();
    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info.
    esp_now_register_recv_cb(OnDataRecv);

    // half duplex, TX only
    Serial1.begin(9600);
}

void loop()
{
    while (send_payload_flag) {
        if (Serial1.availableForWrite()) {
            Serial1.write(espnow_payload);
            send_payload_flag = 0x00;
            digitalWrite(LED_PIN, HIGH);
#ifdef SYS_IN_DEBUG
            Serial.println("Serial1.write(espnow_payload)");
#endif
        }
        delay(5);
    }
    delay(5);
}
