# PSOC&trade; Edge MCU: OTA Wi-Fi HTTPS

This code example demonstrates the firmware update of user application over-the-air (OTA) via the Wi-Fi interface using HTTPS/HTTP protocol, using Arm&reg; Cortex&reg;-M33 CPU on PSOC&trade; Edge MCU coupled with AIROC&trade; CYW55513 Wi-Fi & Bluetooth&reg; combo chip in ModusToolbox&trade;.

The device establishes a HTTPS/HTTP connection with the local Node.js-based web server. It periodically checks the job document to see if a new update is available. When a new update is available, it is downloaded and written to the secondary slot (flash). On the next reboot, MCUboot handles image verification and upgrades. This example uses overwrite-based upgrade. In an overwrite-based upgrade, the new image from the secondary slot is simply copied to the primary slot after successful validation.

This code example has a three project structure: CM33 secure, CM33 non-secure, and CM55 projects. Additionally, add the Edge Protect Bootloader as described in the [Operation](#operation) section. All projects are programmed to the external QSPI flash and executed in Execute in Place (XIP) mode. Extended boot launches the Edge Protect Bootloader from RRAM and the Edge Protect Bootloader launches the CM33 secure project from a fixed location in the external flash, which then configures the protection settings and launches the CM33 non-secure application. CM33 non-secure application handles the OTA task and enables CM55 CPU.

This code example has been tested with the following (out-of-box) configurations:
- Device lifecycle - Development
- Secured boot - Disabled 

See the [Design and implementation](docs/design_and_implementation.md) for the functional description of this code example.


## Requirements

- [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) v3.7 or later (tested with v3.7)
- Board support package (BSP) minimum required version: 1.0.0
- Programming language: C
- Associated parts: All [PSOC&trade; Edge MCU](https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm) parts
- Python (tested with v3.13.7)


## Supported toolchains (make variable 'TOOLCHAIN')

- GNU Arm&reg; Embedded Compiler v14.2.1 (`GCC_ARM`) – Default value of `TOOLCHAIN`
- Arm&reg; Compiler v6.22 (`ARM`)
- IAR C/C++ Compiler v9.50.2 (`IAR`)
- LLVM Embedded Toolchain for Arm&reg; v19.1.5 (`LLVM_ARM`)


## Supported kits (make variable 'TARGET')

- [PSOC&trade; Edge E84 Evaluation Kit](https://www.infineon.com/KIT_PSE84_EVAL) (`KIT_PSE84_EVAL_EPC2`) – Default value of `TARGET`
- [PSOC&trade; Edge E84 Evaluation Kit](https://www.infineon.com/KIT_PSE84_EVAL) (`KIT_PSE84_EVAL_EPC4`)


## Hardware setup

This example uses the board's default configuration. See the kit user guide to ensure that the board is configured correctly.

Ensure the following jumper and pin configuration on board.
- BOOT SW pin (P17.6) must be in the LOW/OFF position
- J20 and J21 must be in the tristate/not connected (NC) position


## Software setup

See the [ModusToolbox&trade; tools package installation guide](https://www.infineon.com/ModusToolboxInstallguide) for information about installing and configuring the tools package.

Install a terminal emulator if you do not have one. Instructions in this document use [Tera Term](https://teratermproject.github.io/index-en.html).

This example requires no additional software or tools.


## Operation

Update firmware using HTTPS

1. **Add Edge Protect Bootloader:** 
   
   By adding *proj_bootloader* to this code example as a primary step. Follow steps 2 to 9 in the **Operation** section of the [Edge Protect Bootloader](https://github.com/Infineon/mtb-example-edge-protect-bootloader) code example's *[README.md](https://github.com/Infineon/mtb-example-edge-protect-bootloader/blob/master/README.md)* file
  
   > **Note:** 
   - [Edge Protect Bootloader](https://github.com/Infineon/mtb-example-edge-protect-bootloader) *README.md* describes how to add the bootloader to **Basic Secure App**. Add it to this code example instead
   - **Edge Protect Bootloader** does not support `LLVM_ARM` Compiler. In the *proj_bootloader* directory, edit *Makefile* to force other supported compilers for Edge Protect Bootloader (for example: `TOOLCHAIN=GCC_ARM`)
 
2. **Configure the memory map:** 

   This code example bundles a custom *design.modus* file compatible with the Edge Protect Bootloader. Therefore, do not make any further customizations here to use Edge Protect Bootloader and this project together

   > **Note:** This code example demonstrates use **overwrite** mode for firmware update

3. **Set up an HTTPS/HTTP server using a local-web-server (based on Node.js):**

   This code example uses a local server to demonstrate the OTA operation over HTTP/HTTPS. In this example, [local-web-server](https://www.npmjs.com/package/local-web-server) is used. It is a lean, modular web server for rapid full-stack development.

   1. Download and install [Node.js](https://nodejs.org/en/download/). Install with the default settings. Do not select to install optional tools for native modules

   2. Open a CLI terminal

      On Linux and macOS, use any terminal application. On Windows, open the *modus-shell* app from the Start menu

   3. Navigate to the *\<My-OTA-HTTPS-example>/scripts/* folder and execute the following command to install [local-web-server](https://www.npmjs.com/package/local-web-server)

       ```
       npm install -g local-web-server
       ```

   4. Start the local HTTP/HTTPS server:

     - **Using the code example in TLS mode** 

        Execute the following command to generate self-signed SSL certificates and keys. On Linux and macOS, get your device-local IP address by running the `ipconfig` command on any terminal application. On Windows, run the `ipconfig` command on a command prompt

	    ```
	    sh generate_ssl_cert.sh <local-ip-address-of-your-pc>
	    ```
      
	    Example:
    
	    ```
	    sh generate_ssl_cert.sh 192.168.0.10
        ```
	   
	    This step will generate the following files in the same *\<OTA_HTTPS>/scripts/* directory:

	    - *http_ca.crt* – Root CA certificate
	    - *http_ca.key* – Root CA private key
	    - *http_server.crt* – Server certificate
	    - *http_server.key* – Server private key
	    - *http_client.crt* – Client certificate
	    - *http_client.key* – Client private key
      
	    Execute the following command to start the server

	    ```
	    ws -p 50007 --hostname <local-ip-address-of-your-pc> --https --key http_server.key --cert http_server.crt --keep-alive-timeout <milli-seconds> -v
	    ```

	    Example:
	    ```
	    ws -p 50007 --hostname 192.168.0.10 --https --key http_server.key --cert http_server.crt --keep-alive-timeout 50000 -v
	    ```
      
	   **Figure 1. HTTPS server started in TLS mode**

	    ![](images/https_tls_mode.png)
      

     - **Using the code example in non-TLS mode** 
   
	   Execute the following command to start the server

	   ```
	   ws -p 50007 --hostname <local-ip-address-of-your-pc> --keep-alive-timeout <milli-seconds> -v
	   ```

	   Example:
	   ```
	   ws -p 50007 --hostname 192.168.0.10 --keep-alive-timeout 50000 -v
	   ```

	   **Figure 2. HTTP server started in non-TLS mode**

	   ![](images/http_non_tls_mode.png)

       > **Note:** If you are running a local-web-server on a device, which is maintained by your organization or institution, the firewall settings may not permit you to host a file server on the local network. To verify whether the file server has been hosted properly from a device connected to the same local network, check the server link on a browser. Browse for `http://<ip-address-noted-earlier>:<port-number-noted-earlier>`; for example, `http://192.168.0.10:8080`. If the files in the *\<OTA_HTTPS>/scripts/* directory are listed on the browser page, you have a properly working file server. Do not proceed to the next section without getting the file server to work

4. **Build and program the bootloader and application image for this code example**

   1. Open the *common.mk* file in the root of the application and set the `COMBINE_SIGN_JSON` variables to the configurator-generated *boot_with_bldr.json* file as follows:

      ```
      COMBINE_SIGN_JSON?=./bsps/TARGET_$(TARGET)/config/GeneratedSource/boot_with_bldr.json
      ```

   2. Ensure `IMG_TYPE` is set to `BOOT` in the same *common.mk* file as follows:

      ```
      IMG_TYPE=BOOT
      ```

   3. Provide your Python path in the *common.mk* file located at the top-level directory of your application (PSOC&trade; Edge MCU: OTA Wi-Fi HTTPS)

      ```
      CY_PYTHON_PATH = <path-to-python>/python.exe
      ```

      > **NOTE:** Install Python if not already installed. If you see the **ModuleNotFoundError: No module named 'click'** error during application build, then you need to install the respective Python module. See the [Quickstart]((https://click.palletsprojects.com/en/stable/quickstart/)) for installation information


   4. Update all the required connection parameters (`WIFI_SSID`, `WIFI_PASSWORD`, `OTA_HTTP_SERVER`, `OTA_HTTP_SERVER_PORT`) in the *\<My-OTA-HTTPS-example>/proj_cm33_ns/app_ota/configs/ota_app_config.h.* configuration file


		**Wi-Fi connection parameters**

		```
		/* Name of the Wi-Fi network. */
		#define WIFI_SSID                            ""

		/* Password for the Wi-Fi network. */
		#define WIFI_PASSWORD                        ""

		/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
		* in "cy_wcm.h" for more details.
		*/
		# define WIFI_SECURITY                      (CY_WCM_SECURITY_WPA2_AES_PSK)                
		```
	   <br>

		```
		/**********************************************
		* HTTP connection parameters
		*********************************************/

		#define OTA_CONNECTION_TYPE                  CY_OTA_CONNECTION_HTTPS 

		#define OTA_UPDATE_FLOW                      CY_OTA_JOB_FLOW

		#define OTA_HTTP_SERVER                      ""

		#define OTA_HTTP_SERVER_PORT                 (50007)

		#define ENABLE_TLS                           1

		#define OTA_HTTP_DATA_FILE                   "/ota-update.tar"

		#define OTA_HTTP_JOB_FILE                    "/ota_update.json"
		```

		> **Note:** Use ENABLE_TLS = 0 and OTA_CONNECTION_TYPE = CY_OTA_CONNECTION_HTTP for non-TLS mode
		<br>

	5. For certificate and keys, navigate to the *\<My-OTA-HTTPS-example>/scripts* directory. Run the `format_cert_key.py` script to generate the string format of the *http_ca.crt*, *http_client.crt*, *http_client.key* as follows:

	   > **Note:** For Linux and macOS platforms, use Python3 instead of Python in the following command
   
		```
		python format_cert_key.py <one-or-more-file-name-of-certificate-or-key-with-extension>
		```
		Example:
		```
		python format_cert_key.py http_ca.crt
		```

	6. Copy the generated string and add it to the certificate macros in the file *\<My-OTA-HTTPS-example>/proj_cm33_ns/app_ota/configs/ota_app_config.h.* file as per the sample shown

		```   
		/**********************************************
		* Certificates and Keys - TLS Mode only
		*********************************************/
		#define ROOT_CA_CERTIFICATE    \
				"-----BEGIN CERTIFICATE-----\n" \
				".........base64 data.......\n" \
				"-----END CERTIFICATE-----\n";



		#define CLIENT_CERTIFICATE  \
				"-----BEGIN CERTIFICATE-----\n" \
				".........base64 data.......\n" \
				"-----END CERTIFICATE-----\n";

		#define CLIENT_KEY     \
				"-----BEGIN RSA PRIVATE KEY-----\n" \
				"...........base64 data.........\n" \
				"-----END RSA PRIVATE KEY-------\n";

		```

	7. Clean, build, and program the application

	8. After programming, the application starts automatically. Confirm that "PSOC Edge MCU : OTA WIFI HTTP Application" is displayed on the UART terminal


		**Figure 3. Terminal output on program startup**

		![](images/terminal_ota_http_boot.png)

		</details>


5. **Build the application image for overwrite update**

   1. Ensure that the following configuration is implemented in the *common.mk* file of this code example. This file can be located in the top-level directory of the application


      Change the image type to update image
      ```
      IMG_TYPE?=UPDATE
      ```
    
	  > **Note:** 
     
	  - The application version will be changed to 1.1.0 in the same *common.mk* file based on `IMG_TYPE`

      - When you select the `IMG_TYPE` to `UPDATE`, the `COMBINE_SIGN_JSON` variable is set automatically in the *common.mk* file, depending on the value of `UPDATE_TYPE`

   2. Clean, build the application **DO NOT PROGRAM** the application. See [Using the code example](docs/using_the_code_example.md) for instructions on creating a project, opening it in various supported IDEs, and performing tasks, such as clean and build the application

   3. Copy the .tar package generated in *\<My-OTA-HTTPS-example>/build* to *\<My-OTA-HTTPS-example>/scripts(Server Path)*

6. **Perform OTA HTTPS firmware update using HTTPS Server**

   1. Open the job document *ota_update.json* in the *\<My-OTA-HTTPS-example>/scripts* directory and check the information
  
      a. Ensure the version is set to **1.1.0**. This version must be different than what has been already programmed on the target

      b. Update the server IP and Port address

			```
			{
			"Message":"Update Available",
			"Manufacturer":"Infineon",
			"ManufacturerId":"IFX",
			"Product":"PSOC EDGE",
			"SerialNumber":"ABC213450001",
			"Version":"1.1.0",
			"Board":"APP_KIT_PSE84_EVAL_EPC2",
			"Connection":"HTTPS",
			"Server":"<IP address of server Hosted>",
			"Port":"<Port on which server is hosted>",
			"File":"/ota-update.tar"
			}
			```
			
		> **Note:** Use "Connection":"HTTP" in job document for Non-TLS mode

   2. Open a CLI terminal

       On Linux and macOS, you can use any terminal application. On Windows, open the 'modus-shell' app from the Start menu.

   3. Navigate to the *\<My-OTA-HTTPS-example>/scripts/* directory

   4. Start the HTTPS/HTTP server 

		```
		ws -p 4443 --hostname 192.168.0.10 --https --key http_server.key --cert http_server.crt --keep-alive-timeout 50000 -v
		```
		> **Note:** See **Step 2** from Set up an HTTPS/HTTP server using local-web-server (based on Node.js) for Non-TLS mode server setup

   5. Reset your kit

   6. OTA update will start automatically since the version in the job document (1.1.0) is higher than the boot image version (1.0.0)


      **Figure 5. Terminal output for OTA job document download start**

      ![](images/http_update_started.png)

      **Figure 6. Terminal output for HTTPS server job document transfer request-response**

      ![](images/http_server_job_started.png)

      **Figure 7. Terminal output for OTA image download start**

      ![](images/http_device_transfer_started.png)

      **Figure 8. Terminal output for OTA image download transfer request-response**

      ![](images/http_server_transfer_started.png)


   7. After image transfer is complete, Edge Protect Bootloader validates the image and copies the image to the primary slot and boots it

      **Figure 9. Terminal output for OTA download completed**

      ![](images/http_device_transfer_complete.png)

      **Figure 10. Terminal output for  HTTPS server OTA transfer completed**

      ![](images/http_server_transfer_complete.png)


      **Figure 11. Terminal output for image validation**

      ![](images/http_bootloader_image_validation.png)

   8. After a successful image upgrade, the following display message will be seen on the terminal window. Verify the application version is for the upgrade image

      **Figure 12. Upgrade image**

      ![](images/terminal_ota_http_update.png)


## Related resources

Resources  | Links
-----------|----------------------------------
Application notes  | [AN235935](https://www.infineon.com/AN235935) – Getting started with PSOC&trade; Edge E8 MCU on ModusToolbox&trade; software
Code examples  | [Using ModusToolbox&trade;](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software) on GitHub
Device documentation | [PSOC&trade; Edge MCU datasheets](https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm#documents) <br> [PSOC&trade; Edge MCU reference manuals](https://www.infineon.com/products/microcontroller/32-bit-psoc-arm-cortex/32-bit-psoc-edge-arm#documents)
Development kits | Select your kits from the [Evaluation board finder](https://www.infineon.com/cms/en/design-support/finder-selection-tools/product-finder/evaluation-board)
Libraries  | [mtb-dsl-pse8xxgp](https://github.com/Infineon/mtb-dsl-pse8xxgp) – Device support library for PSE8XXGP <br> [retarget-io](https://github.com/Infineon/retarget-io) – Utility library to retarget STDIO messages to a UART port <br> [serial-memory](https://github.com/Infineon/serial-memory) - Serial flash middleware for external memory interface <br> [DFU](https://github.com/Infineon/dfu) – Device Firmware Update Middleware (DFU MW) <br> [emusb-device](https://github.com/Infineon/emusb-device) – USB Device stack for embedded applications
Tools  | [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) – ModusToolbox&trade; software is a collection of easy-to-use libraries and tools enabling rapid development with Infineon MCUs for applications ranging from wireless and cloud-connected systems, edge AI/ML, embedded sense and control, to wired USB connectivity using PSOC&trade; Industrial/IoT MCUs, AIROC&trade; Wi-Fi and Bluetooth&reg; connectivity devices, XMC&trade; Industrial MCUs, and EZ-USB&trade;/EZ-PD&trade; wired connectivity controllers. ModusToolbox&trade; incorporates a comprehensive set of BSPs, HAL, libraries, configuration tools, and provides support for industry-standard IDEs to fast-track your embedded application development

<br>


## Other resources

Infineon provides a wealth of data at [www.infineon.com](https://www.infineon.com) to help you select the right device, and quickly and effectively integrate it into your design.


## Document history


Document title: *CE240028* – *PSOC&trade; Edge MCU: OTA Wi-Fi HTTPS* 	

 Version | Description of change
 ------- | ---------------------
 1.x.0   | New code example <br> Early access release
 2.0.0   | GitHub release
 2.1.0   | Added WIFI SDIO DeepSleep Callback <br> Updated design files to fix ModusToolbox™ v3.7 build warnings
<br>


All referenced product or service names and trademarks are the property of their respective owners.

The Bluetooth&reg; word mark and logos are registered trademarks owned by Bluetooth SIG, Inc., and any use of such marks by Infineon is under license.

PSOC&trade;, formerly known as PSoC&trade;, is a trademark of Infineon Technologies. Any references to PSoC&trade; in this document or others shall be deemed to refer to PSOC&trade;.

---------------------------------------------------------

© Cypress Semiconductor Corporation, 2023-2025. This document is the property of Cypress Semiconductor Corporation, an Infineon Technologies company, and its affiliates ("Cypress").  This document, including any software or firmware included or referenced in this document ("Software"), is owned by Cypress under the intellectual property laws and treaties of the United States and other countries worldwide.  Cypress reserves all rights under such laws and treaties and does not, except as specifically stated in this paragraph, grant any license under its patents, copyrights, trademarks, or other intellectual property rights.  If the Software is not accompanied by a license agreement and you do not otherwise have a written agreement with Cypress governing the use of the Software, then Cypress hereby grants you a personal, non-exclusive, nontransferable license (without the right to sublicense) (1) under its copyright rights in the Software (a) for Software provided in source code form, to modify and reproduce the Software solely for use with Cypress hardware products, only internally within your organization, and (b) to distribute the Software in binary code form externally to end users (either directly or indirectly through resellers and distributors), solely for use on Cypress hardware product units, and (2) under those claims of Cypress's patents that are infringed by the Software (as provided by Cypress, unmodified) to make, use, distribute, and import the Software solely for use with Cypress hardware products.  Any other use, reproduction, modification, translation, or compilation of the Software is prohibited.
<br>
TO THE EXTENT PERMITTED BY APPLICABLE LAW, CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO THIS DOCUMENT OR ANY SOFTWARE OR ACCOMPANYING HARDWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  No computing device can be absolutely secure.  Therefore, despite security measures implemented in Cypress hardware or software products, Cypress shall have no liability arising out of any security breach, such as unauthorized access to or use of a Cypress product. CYPRESS DOES NOT REPRESENT, WARRANT, OR GUARANTEE THAT CYPRESS PRODUCTS, OR SYSTEMS CREATED USING CYPRESS PRODUCTS, WILL BE FREE FROM CORRUPTION, ATTACK, VIRUSES, INTERFERENCE, HACKING, DATA LOSS OR THEFT, OR OTHER SECURITY INTRUSION (collectively, "Security Breach").  Cypress disclaims any liability relating to any Security Breach, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any Security Breach.  In addition, the products described in these materials may contain design defects or errors known as errata which may cause the product to deviate from published specifications. To the extent permitted by applicable law, Cypress reserves the right to make changes to this document without further notice. Cypress does not assume any liability arising out of the application or use of any product or circuit described in this document. Any information provided in this document, including any sample design information or programming code, is provided only for reference purposes.  It is the responsibility of the user of this document to properly design, program, and test the functionality and safety of any application made of this information and any resulting product.  "High-Risk Device" means any device or system whose failure could cause personal injury, death, or property damage.  Examples of High-Risk Devices are weapons, nuclear installations, surgical implants, and other medical devices.  "Critical Component" means any component of a High-Risk Device whose failure to perform can be reasonably expected to cause, directly or indirectly, the failure of the High-Risk Device, or to affect its safety or effectiveness.  Cypress is not liable, in whole or in part, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any use of a Cypress product as a Critical Component in a High-Risk Device. You shall indemnify and hold Cypress, including its affiliates, and its directors, officers, employees, agents, distributors, and assigns harmless from and against all claims, costs, damages, and expenses, arising out of any claim, including claims for product liability, personal injury or death, or property damage arising from any use of a Cypress product as a Critical Component in a High-Risk Device. Cypress products are not intended or authorized for use as a Critical Component in any High-Risk Device except to the limited extent that (i) Cypress's published data sheet for the product explicitly states Cypress has qualified the product for use in a specific High-Risk Device, or (ii) Cypress has given you advance written authorization to use the product as a Critical Component in the specific High-Risk Device and you have signed a separate indemnification agreement.
<br>
Cypress, the Cypress logo, and combinations thereof, ModusToolbox, PSoC, CAPSENSE, EZ-USB, F-RAM, and TRAVEO are trademarks or registered trademarks of Cypress or a subsidiary of Cypress in the United States or in other countries. For a more complete list of Cypress trademarks, visit www.infineon.com. Other names and brands may be claimed as property of their respective owners.
