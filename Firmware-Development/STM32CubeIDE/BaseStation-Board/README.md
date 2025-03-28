# STM32WLE5CBU6 Flashing and Testing Guide.

## Table of Content
* [Setting up board](#1---setting-up-board)
* [Install J-Link tools](#2---install-j-link-tools)
* [Setup power supply for the board](#3---setup-power-supply-for-the-board)
* [Connecting the Segger J-Link](#4---connecting-the-segger-j-link)
* [Flashing the MCU using STM32CubeIDE](#5---flashing-the-mcu-using-STM32CubeIDE)


## 1 - Setup up board
Following changes need to be performed on board v1.0 for correct operation.

- Pin8(SWCLK) on U8 header needs to be connected with Pin42(PA14) on U1(STM32WLE5CBU6).
- The LDO is of 1.5 output voltage in schematic but 1.8 in actual. The MCU works but it is removed due to the led's not glowing because of low voltage. Directly supplying 3v3 by removing LDO and injecting 3.3v.
- C7 and C23 needs to be short to supply Vdd to VDDSMPS.

Note: These changes are only valid for board v1.0 and does not apply on v2.0

## 2 - Install J-Link tools

Download J-Link tools from [here](https://www.segger.com/downloads/jlink/)
We used v7.94e. Download the latest from this link for the tools [here](https://www.segger.com/downloads/jlink/JLink_Windows_x86_64.exe)

Follow the installation guide and install.

## 3 - Setup power supply for the board
There are two ways that MCU should be powered depending upon board version.

1 - If V1.0, user has to remove onbaord regulator (which is 1.8v regulator) and apply 3.3v directly onto the output pad of regulator.

2 - If v2.0, apply 4.2v on Vin/+ve terminal on base-board.

## 4 - Connecting the Segger J-Link
The programming pins are extending to 6 pin Male-Header(U8) which should be connected with the J-Link Interface board female headers. Make sure to align the ground (Black wire) to the ground of J-Link Inteface board female header which are also labeled accordingly.
**Make sure the debugger 10pin ribbon connector is attached in correct orientation as shown in the image below.**

## 5 - Flashing the MCU using STM32CubeIDE.
Open the project in **STM32CubeIDE**.

**main.cpp** file is located in the **Core/Src** directory.
Click on **Debug As** option located on the top ribbon under menu tab.
then flash the hex output using **Run As** option besides **Debug As** option.

It should show successfully flashed message.

If MCU fails to start due to bad wiring, flashing will fail


