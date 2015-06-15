# YARD-ICE Firmware Upgrade #

## Introduction ##

One of the nice features of the STM32 family of microcontrollers is the embedded flash loader.

ST provides a demo application that can be used to upload the firmware using the USB connection.

## How To Upload ##

  * Download the [DfuSe USB device firmware upgrade STMicroelectronics extension](http://www.st.com/st-web-ui/static/active/en/st_prod_software_internet/resource/technical/software/demo_and_example/stsw-stm32080.zip).
  * Decompress the ZIP file:
> ![http://yard-ice.googlecode.com/files/dfuse-7zip-stsw-stm32080-zip.png](http://yard-ice.googlecode.com/files/dfuse-7zip-stsw-stm32080-zip.png)
  * Run the appropriated installer for your platform:
    * 32 bits: DfuSe\_Demo\_V3.0.3\_Setup.exe
    * 64 bits: DfuSe\_Demo\_V3.0.3\_Setup\_amd64.exe
  * Disconnect the power and USB from the YARD-ICE
  * Insert the P2 jumper in YARD-ICE board
  * Connect the USB cable
  * Run the `STMicroelectronics->DfuSe->DfuSe Demonstration`
> ![https://yard-ice.googlecode.com/files/dfuse-demo-v303.png](https://yard-ice.googlecode.com/files/dfuse-demo-v303.png)
  * The **`DfuSe Demo`** application should show `STM Device in DFU Mode` under the `Available DFU Devices`
  * Under "Upgrade or Veriy Action" press the `[Choose...]` button
  * Select the **yard-yce.dfu** firmware file. See the next section on how to create the DFU file.
  * Press the `[Upgrade]` button

  * After writing the firmware, power cycle the system without jumper P2.

## How To Create a DFU File ##

The ST utility requires the firmware to be packed in a proprietary FDU format. There is one utility provided in the demo that allows to convert the output binary into a FDU file format.
Here is how to do it:
  * Run the `STMicroelectronics->DfuSe->DFU File Manager`
> ![http://yard-ice.googlecode.com/files/dfu-want-to.png](http://yard-ice.googlecode.com/files/dfu-want-to.png)
  * Select the option `I want to GENERATE a DFU file form S19, HEX or BIN files`
> ![http://yard-ice.googlecode.com/files/dfu-file-manager.png](http://yard-ice.googlecode.com/files/dfu-file-manager.png)
  * Select the button `[Multi BIN...]`
  * Select the **yard-ice.bin** binary file from the **src/release** directory.
> ![http://yard-ice.googlecode.com/files/dfu-multi-bin.png](http://yard-ice.googlecode.com/files/dfu-multi-bin.png)
  * Set the address to **`0x08000000`**, press the `[Add to List>>]` button and then `OK` to close this window.
  * Press the `[Generate...]` button to create the **yard-ice.dfu** file.