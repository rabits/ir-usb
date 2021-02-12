#!/bin/sh

g++ -pthread -o ir-usb src/ir-usb.c src/TiqiaaUsb.cpp -lusb-1.0
