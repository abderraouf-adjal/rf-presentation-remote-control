/* SPDX-License-Identifier: ISC */
/**
 * Licensing and copyright: See the file `LICENSE.txt`
 */

/**
 * Project name: Wireless Presentation Remote Control - The master/sender with buttons
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
 *   - 2 push-buttons (UP button & DOWN button).
 * # Software:
 *   - ESP8266 Arduino Core (Framework), tested with v2.7.4 (https://github.com/esp8266/Arduino)
 * # NOTES:
 *   - IO Configs:     Built-in LED: GPIO-2/D4.
 *                     UP button:   BTN_UP_PIN/GPIO-5/D1
 *                     DOWN button: BTN_DW_PIN/GPIO-4/D2
 */

//#define SYS_IN_DEBUG

#define LED_PIN 2 /* D4 */
#define BTN_UP_PIN D1 /* 5 */
#define BTN_DW_PIN D2 /* 4 */

#define ESPNOW_CHANNEL 14

// master/sender STA MAC (D1 Mini): 3E:71:BF:26:6D:00
uint8_t thisMac[] = { 0x3E, 0x71, 0xBF, 0x26, 0x6D, 0x00 }; // Replace with the AP MAC address of the slave/receiver
// remoteMac (NodeMCU): AP MAC: 86:F3:EB:1A:C5:01
uint8_t remoteMac[] = { 0x86, 0xF3, 0xEB, 0x1A, 0xC5, 0x01 }; // Replace with the AP MAC address of the slave/receiver

#include <ESP8266WiFi.h>
#include <espnow.h>
#include <user_interface.h>

volatile uint8_t espnow_payload = 0;
volatile uint8_t espnow_payload_counter = 0;
volatile uint8_t espnow_try_send = 0;
volatile uint8_t espnow_netstatus = 0;

// Init ESP Now with fallback
void InitESPNow()
{
    if (esp_now_init() == 0) {
#ifdef SYS_IN_DEBUG
        Serial.println("ESPNow Init Success");
#endif
    } else {
#ifdef SYS_IN_DEBUG
        Serial.println("ESPNow Init Failed");
        Serial.flush();
#endif
        ESP.restart();
    }
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
}

void sendData()
{
    digitalWrite(LED_PIN, LOW);

    uint8_t tmp_payload[2] = { espnow_payload, espnow_payload_counter };
    int result = esp_now_send(remoteMac, (uint8_t*)tmp_payload, (int)sizeof(tmp_payload));
#ifdef SYS_IN_DEBUG
    if (result == 0) {
        Serial.println("Send Command: Success " + String(result));
    } else {
        Serial.println("Send Command: Failed " + String(result));
    }
#endif
}

// callback when data is sent from Master to Slave
void ICACHE_FLASH_ATTR OnDataSent(u8* mac_addr, u8 status)
{
#ifdef SYS_IN_DEBUG
    /*char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print("Packet Sent to MAC: ");
    Serial.println(macStr);*/

    if (status == 0) {
        Serial.println("ESPNOW: ACK_OK");
    } else {
        Serial.println("ESPNOW: SEND_FAILED");
    }
#endif

    if (status == 0) {
        // Sending Success
        digitalWrite(LED_PIN, HIGH);
        espnow_payload_counter++;
        espnow_netstatus = 1; // Signal callback with seccess
        //espnow_try_send = 0; // Signal sending done with success
    } else {
        // Sending Failed
        espnow_netstatus = 2; // Signal callback with failure
    }
}

/* Pin BTN UP ISR. */
void ICACHE_RAM_ATTR ISR_btn_up(void)
{
    ETS_GPIO_INTR_DISABLE();
    if (!espnow_try_send) {
        espnow_payload = 'U';
        espnow_try_send = 0xFF;
    }
    ETS_GPIO_INTR_ENABLE();
}

/* Pin BTN DW ISR. */
void ICACHE_RAM_ATTR ISR_btn_dw(void)
{
    ETS_GPIO_INTR_DISABLE();
    if (!espnow_try_send) {
        espnow_payload = 'D';
        espnow_try_send = 0xFF;
    }
    ETS_GPIO_INTR_ENABLE();
}

void initVariant(void)
{
    //WiFi.persistent(false);
    wifi_set_macaddr(STATION_IF, &thisMac[0]);
}


void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);
    pinMode(BTN_UP_PIN, INPUT_PULLUP);
    pinMode(BTN_DW_PIN, INPUT_PULLUP);

    Serial.begin(115200);
#ifndef SYS_IN_DEBUG
    Serial.flush();
    Serial.end();
#endif

    //Set device in STA mode to begin with
    WiFi.mode(WIFI_STA);
    //WiFi.disconnect();
    WiFi.setSleepMode(WIFI_MODEM_SLEEP);
    //wifi_set_sleep_type(LIGHT_SLEEP_T);
    

#ifdef SYS_IN_DEBUG
    // This is the mac address of the Master in Station Mode
    Serial.print("STA MAC: ");
    Serial.println(WiFi.macAddress());
#endif

    /* Init ESPNow with a fallback logic */
    InitESPNow();

    /* Once ESPNow is successfully Init, we will register for Send CB to
     * get the status of Trasnmitted packet
     */
    esp_now_register_send_cb(OnDataSent);
    //int addStatus = esp_now_add_peer((u8*)remoteMac, ESP_NOW_ROLE_CONTROLLER, ESPNOW_CHANNEL, NULL, 0);
    int addStatus = esp_now_add_peer((u8*)remoteMac, ESP_NOW_ROLE_SLAVE, ESPNOW_CHANNEL, NULL, 0);
    if (addStatus != 0) {
        /* Pair failed */
#ifdef SYS_IN_DEBUG
        Serial.println("Pair failed");
        Serial.flush();
#endif
        ESP.restart();
    } else {
        /* Pair success */
#ifdef SYS_IN_DEBUG
        Serial.println("Pair success");
#endif
    }

    attachInterrupt(digitalPinToInterrupt(BTN_UP_PIN), ISR_btn_up, FALLING);
    attachInterrupt(digitalPinToInterrupt(BTN_DW_PIN), ISR_btn_dw, FALLING);
}


void loop()
{
    while (espnow_try_send) {
#ifdef SYS_IN_DEBUG
        Serial.println("sendData()");
#endif
        sendData();
        // Wait for net callback flag
        while (! espnow_netstatus) {
            delay(20);
        }

        // Anti button debounce
        delay(100); 

        if (espnow_netstatus == 1) { // seccess
            // Anti button debounce
            while ((digitalRead(BTN_UP_PIN) == LOW) || (digitalRead(BTN_DW_PIN) == LOW)) {
                delay(100);
            }
            espnow_try_send = 0x00; // Signal sending done with success
        }

        // Reset netstatus flag
        espnow_netstatus = 0;
    }
    
    delay(20);
}
