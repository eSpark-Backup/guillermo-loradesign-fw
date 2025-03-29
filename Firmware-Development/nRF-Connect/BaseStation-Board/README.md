# NRF Flashing and Testing Guide.

## Table of Content
* [Setup NRF connect SDK on VS Code](#1---setup-nrf-connect-sdk-on-vs-code)
    - [VS Code Installation](#a-vs-code-installation)
    - [Add NRF Connect SDK extension in VS Code](#b-add-nrf-connect-sdk-extension-in-vs-code)
        - [Install Toolchain](#i-install-toolchain)
        - [Install SDK](#ii-install-sdk)
* [Install J-Link tools](#2---install-j-link-tools)
* [Setup power supply for the board](#3---setup-power-supply-for-the-board)
* [Connecting the Segger J-Link](#4---connecting-the-segger-j-link)
* [Flashing the MCU using NRF Connect SDK from VS Code](#5---flashing-the-mcu-using-nrf-connect-sdk-from-vs-code)


## 1 - Setup NRF connect SDK on VS Code
###  a) VS Code Installation

1. Download the [Visual Studio Code installer](https://go.microsoft.com/fwlink/?LinkID=534107) for Windows.
2. Once it is downloaded, run the installer (VSCodeUserSetup-{version}.exe). This will only take a minute.
3. By default, VS Code is installed under `C:\Users\{Username}\AppData\Local\Programs\Microsoft VS Code`.

Alternatively, you can also download a [Zip archive](https://code.visualstudio.com/docs/?dv=winzip), extract it and run Code from there.

>**Tip:** Setup will add Visual Studio Code to your `%PATH%`, so from the console you can type 'code .' to open VS Code on that folder. You will need to restart your console after the installation for the change to the `%PATH%` environmental variable to take effect.

### b) Add NRF Connect SDK extension in VS Code.
Add both the nrf connect sdk extensions.
After installing the extensions nrf connect extension icon will appear on the left bar.

#### i) Install Toolchain
Click on Manage toolchain, then install the nRF Connect SDK Toolchain v2.9.0.

#### ii) Install SDK
Then do the same for Manage SDKs, install v2.9.0


## 2 - Install J-Link tools

Download J-Link tools from [here](https://www.segger.com/downloads/jlink/)
We used v7.94e. Download the latest from this link for the tools [here](https://www.segger.com/downloads/jlink/JLink_Windows_x86_64.exe)

Follow the installation guide and install.

## 3 - Setup power supply for the board
There are two ways that MCU should be powered depending upon board version.

1 - If V1.0, user has to remove onbaord regulator (which is 1.8v regulator) and apply 3.3v directly onto the output pad of regulator.

2 - If v2.0, apply 4.2v on Vin/+ve terminal on base-board.

## 4 - Connecting the Segger J-Link
The programming pins are extending to 6 pin Male-Header(U6) which should be connected with the J-Link Interface board female headers. Make sure to align the ground (Black wire) to the ground of J-Link Inteface board female header which are also labeled accordingly.
**Make sure the debugger 10pin ribbon connector is attached in correct orientation.**

## 5 - Flashing the MCU using NRF Connect SDK from VS Code.

> [!IMPORTANT]
> Make sure to apply these configurations before flashing in the `prj.conf` file.
>1) Set RC Oscillator as Clock Source  
>`CONFIG_CLOCK_CONTROL_NRF_K32SRC_RC=y`


On the VS Code click on **Open an existing application** and open the application.

**Main.c** file is located in the **src** directory.
Click on **Build** option located on the bottom tab called **NRF:CONNECT: ACTIONS:**
then flash the hex output using **Flash** option below **Build** option.

It should show successfully flashed message.

If MCU fails to start due to bad wiring, flashing will fail

If flashed successfully you can view the logs on the RTT Viewer which comes in the J-Link Tools.


