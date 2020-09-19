#include "main.h"

void sendTestMessage() {
    setTestMessage();
    digitalWriteFast(13, HIGH);
    Can0.write(test_msg);
    digitalWriteFast(13, LOW);
    // Serial.println("Send");
    // Serial.print("Send Buffer: ");
    // for (uint8_t i = 0; i < test_msg.len; i++) {
    //     Serial.print(test_msg.buf[i], HEX);
    //     Serial.print(" ");
    // }
    // Serial.println();
}

void setup(void) {
    Serial.begin(115200);
    delay(1000);
    pinMode(13, OUTPUT);
    pinMode(6, OUTPUT);
    digitalWrite(6, LOW); /* optional tranceiver enable pin */

    // FlexCan
    Can0.begin();
    Can0.setBaudRate(1000000);
    Can0.setMaxMB(NUM_TX_MAILBOXES + NUM_RX_MAILBOXES);
    //Can0.enableFIFO();
    //Can0.enableFIFOInterrupt();
    Can0.onReceive(FIFO, canSniff);
    for (uint8_t i = 0; i < NUM_RX_MAILBOXES; i++) {
        Can0.setMB((FLEXCAN_MAILBOX)i, RX, NONE);
    }
    for (uint8_t i = NUM_RX_MAILBOXES; i < (NUM_TX_MAILBOXES + NUM_RX_MAILBOXES); i++) {
        Can0.setMB((FLEXCAN_MAILBOX)i, TX, NONE);
    }
    Can0.setMBFilter(REJECT_ALL);
    Can0.enableMBInterrupts();

// Set mailbox filters & handlers from def file
#define X(MB, add, func)                        \
    Can0.setMBFilter((FLEXCAN_MAILBOX)MB, add); \
    Can0.enhanceFilter((FLEXCAN_MAILBOX)MB);    \
    Can0.onReceive((FLEXCAN_MAILBOX)MB, func);
    CAN_MESSAGES
#undef X
    Can0.mailboxStatus();
}

void loop() {
    Can0.events();
    static uint32_t timeout = millis();
    if (millis() - timeout > 500) {
        sendTestMessage();
        timeout = millis();
    }
}

int main(void) {
    setup();
    while (1) {
        loop();
    }
}
