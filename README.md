# Smart Home Project

## Table of Contents

- [Introduction](#introduction)
- [Features](#features)
- [Components](#components)
- [Demo](#demo)

## Introduction

Welcome to the Smart Home Project! This project allows you to control your home lighting system through a client app. It's a great way to start building your own smart home infrastructure.

## Features

- **Light Control**: Toggle lights on and off remotely using the client app.
- **More coming soon**

## Components

![Smart Home Components](doc_assets/components.jpg)

The main goal of this project is to utilize the ESP32-C6's support for Zigbee networks to create an efficient and reliable smart home solution. It aims to leverage the ESP32-C6's capabilities to seamlessly connect and control Zigbee-enabled devices, providing users with enhanced convenience and automation in their homes.

The client app (_smart_home_client_app_) connects to the server (_python_socket_mqtt_) that is running on a Raspberry Pi 3B+, running Raspbian GNU/Linux 11 OS. It listens to client requests using TCP sockets. On the same server there is an instance of a MQTT client which forwards the messages from client to the ESP32 acting as a gateway (_zb_wifi_gateway_). The gateway device then forwards the client messages via UART protocol to Zigbee coordinator(_zb_coordinator_) which creates a Zigbee network and controls Zigbee end devices (_zb_end_device_).

The purpose of the gateway device is to have only Zigbee network running on an ESP32-C6 device because of the potential coexsistence problems when using Wi-Fi and Zigbee at the same time, as ESP32-C6 has only one 2.4 GHz ISM band RF module. These issues were resolved as of [March 2023](https://github.com/espressif/esp-idf/issues/10423), but there still remains a question of performance when having multiple services running.

## Demo

![Smart Home Demo](doc_assets/system_demo.gif)
