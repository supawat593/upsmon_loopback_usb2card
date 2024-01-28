/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "./TimerTPL5010/TimerTPL5010.h"
#include "USBSerial.h"
#include "devices_src.h"
#include "mbed.h"
#include <cstdint>

// Blinking rate in milliseconds
#define BLINKING_RATE 500ms

DigitalOut led(LED1);
TimerTPL5010 tpl5010(WDT_WAKE_PIN, WDT_DONE_PIN);
UnbufferedSerial ecard(ECARD_TX_PIN, ECARD_RX_PIN, 2400);
UnbufferedSerial ecard2(MDM_TXD_PIN, MDM_RXD_PIN, 2400);
USBSerial pc;

Thread isr_thread(osPriorityAboveNormal, 0x400, nullptr, "isr_queue_thread");
Thread usb_thread;

EventQueue isr_queue;

void on_rx_interrupt();
void on_rx_interrupt2();

void usb_task() {
  printf("Start usb_task\r\n");

  while (true) {
    while (pc.connected()) {
      if (pc.readable()) {
        uint8_t byte = pc.getc();
        // putc(byte, stdout);
        ecard.write((void *)&byte, 1);
        ecard2.write((void *)&byte, 1);
      }
    }
  }
}
int main() {
  // Initialise the digital pin LED1 as an output
  printf("start Loopback_ecard\r\n");

  isr_thread.start(callback(&isr_queue, &EventQueue::dispatch_forever));
  tpl5010.init(&isr_queue);
  ecard.attach(callback(on_rx_interrupt), SerialBase::RxIrq);
  ecard2.attach(callback(on_rx_interrupt2), SerialBase::RxIrq);
  usb_thread.start(callback(usb_task));

  while (true) {
    if (tpl5010.get_wdt()) {
      tpl5010.set_wdt(false);
    }

    led = !led;
    ThisThread::sleep_for(BLINKING_RATE);
  }
}

void on_rx_interrupt() {
  char c;

  // Read the data to clear the receive interrupt.
  if (ecard.read(&c, 1)) {
    // Echo the input back to the terminal.
    pc.write((void *)&c, 1);
  }
}

void on_rx_interrupt2() {
  char c;

  // Read the data to clear the receive interrupt.
  if (ecard2.read(&c, 1)) {
    // Echo the input back to the terminal.
    pc.write((void *)&c, 1);
  }
}
