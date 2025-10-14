/*****************************************************************************
* File Name        : ota_task.c
*
* Description      : This source file contains the ota task defintion
*
* Related Document : See README.md
*
*******************************************************************************
* (c) 2024-2025, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
* 
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*******************************************************************************/

/*******************************************************************************
* Header Files
*******************************************************************************/
#include "cybsp.h"

/* FreeRTOS header file */
#include "FreeRTOS.h"
#include "task.h"
#include "cybsp_wifi.h"

/* Wi-Fi connection manager */
#include "cy_wcm.h"
/* OTA storage api */
#include "cy_ota_storage_api.h"
#include "ota_app_config.h"

#include "lwip/netif.h"
#include "ota_app_config.h"
/*******************************************************************************
* Macros
********************************************************************************/
#define SDHC_SDIO_64B_BLOCK                 (64U)
#define APP_SDIO_INTERRUPT_PRIORITY         (7U)
#define APP_SDIO_FREQUENCY_HZ               (25000000U)

/*******************************************************************************
* Global Variables
*******************************************************************************/
/* OTA context */
cy_ota_context_ptr ota_context;
cy_stc_sd_host_context_t sdhc_host_context;
mtb_hal_sdio_t sdio_instance;
/**
 * @brief HTTP Credentials for OTA
 */
cy_awsport_ssl_credentials_t    http_credentials;

/**
 * @brief network parameters for OTA
 */
cy_ota_network_params_t     ota_network_params = { CY_OTA_CONNECTION_UNKNOWN };

/**
 * @brief aAgent parameters for OTA
 */
cy_ota_agent_params_t     ota_agent_params = { 0 };

/**
 * @brief Storage interface APIs for storage operations.
 */
cy_ota_storage_interface_t ota_interfaces =
{
   .ota_file_open            = cy_ota_storage_open,
   .ota_file_read            = cy_ota_storage_read,
   .ota_file_write           = cy_ota_storage_write,
   .ota_file_close           = cy_ota_storage_close,
   .ota_file_verify          = cy_ota_storage_verify,
   .ota_file_validate        = cy_ota_storage_image_validate,
   .ota_file_get_app_info    = cy_ota_storage_get_app_info
};



/*******************************************************************************
* Function Prototypes
********************************************************************************/
void ota_task(void *arg);
cy_rslt_t initialize_ota(void);
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data);
static cy_rslt_t wifi_connect(void);
static cy_wcm_config_t wifi_config;

/******************************************************
 *               Function Definitions
 ******************************************************/

/*******************************************************************************
 * Function Name: ota_task
 *******************************************************************************
 * Summary:
 *  Task used to establish a connection to a remote TCP client.
 *
 * Parameters:
 *  void *args : Task parameter defined during task creation (unused)
 *
 * Return:
 *  void
 *
 *******************************************************************************/
void ota_task(void * arg)
{
    cy_rslt_t result;
    uint32_t image_ID;

    wifi_config.interface = CY_WCM_INTERFACE_TYPE_STA;
    wifi_config.wifi_interface_instance = &sdio_instance;

    // Disable watchdog to prevent wakeup from deep sleep mode
    Cy_WDT_Unlock();
    Cy_WDT_Disable();
    Cy_WDT_Lock();
    
    /* Initialize external flash */
    result = cy_ota_storage_init();

    /* External flash init failed. Stop program execution */
    if (result!=CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Validate all three images so that swap is permanent */
    for ( image_ID = 1; image_ID < 4; image_ID++)
    {
        cy_ota_storage_image_validate(image_ID);
    }
    
    /* Initialize Wi-Fi connection manager. */
    result = cy_wcm_init(&wifi_config);
    

    printf("===============================================================\n");

    if( CY_RSLT_SUCCESS == result)
    {
        printf("Wifi module Initialization Successful\n");
    }
    else
    {
        printf("Failed to initialize Wi-Fi Connection Manager.\n");
        printf("Error: \n------------------------------------------------------------------------------\n"
               "Init Failed\n"
               "This version of the code example only supports EVK versions REV *C and above that contain the latest revision of CYW55513 device.\n"
               "The revision of CYW55513 is printed in the messages above and it should be 'chip rev: 1' for this code example to work.\n"
               "Verify your EVK version otherwise contact Infineon representative to get the latest hardware.\n"
               "It could be hardware issue if you are using the latest hardware and still see this error.\n"
               "-------------------------------------------------------------------------------\n");

        CY_ASSERT(0);
    }

    printf("===============================================================\n");

    /* Connect to Wi-Fi */
    if(CY_RSLT_SUCCESS != wifi_connect())
    {
        printf("\n Wi-Fi connection failed.\n");
        CY_ASSERT(0);
    }

    /* Initialize underlying support code that is needed for OTA and MQTT */
    if (cy_awsport_network_init() != CY_RSLT_SUCCESS)
    {
        printf("Initializing secure sockets Failed.\n");
        CY_ASSERT(0);
    }


    /* Initialize and start the OTA agent */
    if(CY_RSLT_SUCCESS != initialize_ota())
    {
        printf("\n Initializing and starting the OTA agent failed.\n");
        CY_ASSERT(0);
    }

    vTaskSuspend(NULL);
}
/*******************************************************************************
 * Function Name: initialize_ota()
 *******************************************************************************
 * Summary:
 *  Initialize and start the OTA update
 *
 * Parameters:
 *  pointer to Application context
 *
 * Return:
 *  cy_rslt_t
 *
 *******************************************************************************/
cy_rslt_t initialize_ota(void)
{
    cy_rslt_t               result;

    memset(&ota_network_params, 0, sizeof(ota_network_params));
    memset(&ota_agent_params,   0, sizeof(ota_agent_params));



    /* Network Parameters */
    ota_network_params.initial_connection    = OTA_CONNECTION_TYPE;
    ota_network_params.use_get_job_flow      = OTA_UPDATE_FLOW;


    /* HTTP Connection values */
    ota_network_params.http.server.host_name = OTA_HTTP_SERVER;  

    ota_network_params.http.server.port = OTA_HTTP_SERVER_PORT;


    /* For HTTP we change the file we GET when using a job flow */
    if (ota_network_params.use_get_job_flow == CY_OTA_JOB_FLOW)
    {
        /* For HTTP server to get "job" file */
        ota_network_params.http.file = OTA_HTTP_JOB_FILE;
    }
    else
    {
        /* For HTTP server to get "data" file directly */
        ota_network_params.http.file = OTA_HTTP_DATA_FILE;
    }

    /* set up HTTP credentials - only used if start_TLS is set */
    memset(&ota_network_params.http.credentials, 0x00, sizeof(ota_network_params.http.credentials ));

    http_credentials.alpnprotos         = NULL;
    http_credentials.alpnprotoslen      = 0;
    http_credentials.sni_host_name      = NULL;
    http_credentials.sni_host_name_size = 0;
    http_credentials.root_ca            = ROOT_CA_CERTIFICATE;
    http_credentials.root_ca_size       = strlen(http_credentials.root_ca) + 1;
    http_credentials.client_cert        = CLIENT_CERTIFICATE;
    http_credentials.client_cert_size   = strlen(http_credentials.client_cert) + 1;
    http_credentials.private_key        = CLIENT_KEY;
    http_credentials.private_key_size   = strlen(http_credentials.private_key) + 1;

    ota_network_params.http.credentials = http_credentials;

    /* OTA Agent parameters */
    ota_agent_params.validate_after_reboot  = 1;  /* 1 = validate the image after reboot */
    ota_agent_params.reboot_upon_completion = 1;  /* 1 = reboot when download is finished */
    ota_agent_params.cb_func                = ota_callback;
    ota_agent_params.cb_arg                 = &ota_context;
    ota_agent_params.do_not_send_result     = 1;  /* 1 = do not send result */

    result = cy_ota_agent_start(&ota_network_params, &ota_agent_params, &ota_interfaces, &ota_context);
    if (result != CY_RSLT_SUCCESS)
    {
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_ERR, "cy_ota_agent_start() Failed - result: 0x%lx\n", result);
    }
    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "cy_ota_agent_start() Result: 0x%lx\n", result);

    return result;
}
/******************************************************************************
 * Function Name: wifi_connect
 ******************************************************************************
 * Summary:
 *  Function that initiates connection to the Wi-Fi Access Point using the
 *  specified SSID and PASSWORD. The connection is retried a maximum of
 *  'MAX_WIFI_CONN_RETRIES' times with interval of 'WIFI_CONN_RETRY_INTERVAL_MS'
 *  milliseconds.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t : CY_RSLT_SUCCESS upon a successful Wi-Fi connection, else an
 *              error code indicating the failure.
 *
 ******************************************************************************/
static cy_rslt_t wifi_connect(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_wcm_connect_params_t connect_param;
    cy_wcm_ip_address_t ip_address;

    /* Check if Wi-Fi connection is already established. */
    if (!(cy_wcm_is_connected_to_ap()))
    {
        /* Configure the connection parameters for the Wi-Fi interface. */
        memset(&connect_param, 0, sizeof(cy_wcm_connect_params_t));
        memcpy(connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
        memcpy(connect_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
        connect_param.ap_credentials.security = WIFI_SECURITY;

        printf("\nWi-Fi Connecting to '%s'\n", connect_param.ap_credentials.SSID);

        /* Connect to the Wi-Fi AP. */
        for (uint32_t retry_count = 0; retry_count < MAX_WIFI_CONN_RETRIES; retry_count++)
        {
            result = cy_wcm_connect_ap(&connect_param, &ip_address);

            if (CY_RSLT_SUCCESS == result)
            {
                printf("\nSuccessfully connected to Wi-Fi network '%s'.\n", connect_param.ap_credentials.SSID);

                if (ip_address.version == CY_WCM_IP_VER_V4)
                {
                    printf("IPv4 Address Assigned: %s\n\n", ip4addr_ntoa((const ip4_addr_t *) &ip_address.ip.v4));
                }
                else if (ip_address.version == CY_WCM_IP_VER_V6)
                {
                    printf("IPv6 Address Assigned: %s\n\n", ip6addr_ntoa((const ip6_addr_t *) &ip_address.ip.v6));
                }
                return result;
            }

            printf("Wi-Fi Connection failed. Error code:0x%0X. Retrying in %d ms. Retries left: %d\n",
                (int)result, WIFI_CONN_RETRY_INTERVAL_MS, (int)(MAX_WIFI_CONN_RETRIES - retry_count - 1));
            vTaskDelay(pdMS_TO_TICKS(WIFI_CONN_RETRY_INTERVAL_MS));
        }

        printf("\nExceeded maximum Wi-Fi connection attempts!\n");
    }
    return result;
}
/*******************************************************************************
 * Function Name: ota_callback()
 *******************************************************************************
 * Summary:
 *  Got a callback from OTA
 *
 *  @param[in][out] cb_data - structure holding information for the Application
 *
 * Return:
 *  CY_OTA_CB_RSLT_OTA_CONTINUE - OTA Agent to continue with function, using modified data from Application
 *  CY_OTA_CB_RSLT_OTA_STOP     - OTA Agent to End current update session (do not quit).
 *  CY_OTA_CB_RSLT_APP_SUCCESS  - Application completed task, OTA Agent use success
 *  CY_OTA_CB_RSLT_APP_FAILED   - Application completed task, OTA Agent use failure
 *
 *******************************************************************************/
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data)
{
    cy_ota_callback_results_t   cb_result = CY_OTA_CB_RSLT_OTA_CONTINUE;    /* OTA Agent to continue as normal */
    const char                  *state_string;
    const char                  *error_string;
    const char                  *reason_string;


    if (cb_data == NULL)
    {
        return CY_OTA_CB_RSLT_OTA_STOP;
    }

    state_string  = cy_ota_get_state_string(cb_data->ota_agt_state);
    error_string  = cy_ota_get_error_string(cy_ota_get_last_error());
    reason_string = cy_ota_get_callback_reason_string(cb_data->reason);

    /* Normal OTA callbacks here */
    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG, "APP OTA CB NORMAL: state:%d  %s reason:%d %s\n",
                cb_data->ota_agt_state, state_string, cb_data->reason, reason_string);

    switch (cb_data->reason)
    {

    case CY_OTA_LAST_REASON:
        break;

    case CY_OTA_REASON_SUCCESS:
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB SUCCESS state:%d %s last_error:%s\n", cb_data->ota_agt_state, state_string, error_string);
        break;
    case CY_OTA_REASON_FAILURE:
        cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB FAILURE state:%d %s last_error:%s\n", cb_data->ota_agt_state, state_string, error_string);
        break;
    case CY_OTA_REASON_STATE_CHANGE:

        switch (cb_data->ota_agt_state)
        {
            /* informational */
        case CY_OTA_STATE_NOT_INITIALIZED:
        case CY_OTA_STATE_EXITING:
        case CY_OTA_STATE_INITIALIZING:
        case CY_OTA_STATE_AGENT_STARTED:
        case CY_OTA_STATE_AGENT_WAITING:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STATE CHANGE state:%d %s last_error:%s\n", cb_data->ota_agt_state, state_string, error_string);
            break;

        case CY_OTA_STATE_START_UPDATE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STATE CHANGE CY_OTA_STATE_START_UPDATE\n");
            break;

        case CY_OTA_STATE_STORAGE_OPEN:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STORAGE OPEN\n");
            break;

        case CY_OTA_STATE_STORAGE_WRITE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_NOTICE, "APP OTA CB STORAGE WRITE %ld%% (%ld of %ld)\n", cb_data->percentage, cb_data->bytes_written, cb_data->total_size);
            break;

        case CY_OTA_STATE_STORAGE_CLOSE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB STORAGE CLOSE\n");
            break;

        case CY_OTA_STATE_JOB_CONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB CONNECT FOR JOB\n");
            /* NOTE:
             *  HTTP - json_doc holds the HTTP "GET" request
             */

            if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            {
                if ((cb_data->broker_server.host_name == NULL) ||
                    ( cb_data->broker_server.port == 0) ||
                    ( strlen(cb_data->file) == 0) )
                {
                    cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "ERROR in callback data: HTTP: server: %p port: %d topic: '%p'\n",
                            cb_data->broker_server.host_name, cb_data->broker_server.port, cb_data->file);
                    return CY_OTA_CB_RSLT_OTA_STOP;
                }
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: server:%s port: %d file: '%s'\n",
                        cb_data->broker_server.host_name, cb_data->broker_server.port, cb_data->file);
            }


            break;
        case CY_OTA_STATE_JOB_DOWNLOAD:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB JOB DOWNLOAD using:\n");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "GET" request
             */

                if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            {
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: '%s'\n", cb_data->file);
            }
            break;

        case CY_OTA_STATE_JOB_DISCONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB JOB DISCONNECT\n");

            break;

        case CY_OTA_STATE_JOB_PARSE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, " APP OTA CB PARSE JOB\n");

            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "      '%.*s' \n", strlen(cb_data->json_doc), cb_data->json_doc);

            break;

        case CY_OTA_STATE_JOB_REDIRECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "TODO: APP OTA CB JOB REDIRECT\n");
            break;

        case CY_OTA_STATE_DATA_CONNECT:

            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB CONNECT FOR DATA\n");

            break;

        case CY_OTA_STATE_DATA_DOWNLOAD:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "APP OTA CB DATA DOWNLOAD using:\n");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "GET" request
             */
if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            
            {

                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "HTTP: '%s' \n", cb_data->json_doc);
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_DEBUG2, "                        file: '%s' \n", cb_data->file);
                }
            break;

        case CY_OTA_STATE_DATA_DISCONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB DATA DISCONNECT\n");

            break;

        case CY_OTA_STATE_VERIFY:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB VERIFY\n");

            break;

        case CY_OTA_STATE_RESULT_REDIRECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB RESULT REDIRECT\n");

            break;

        case CY_OTA_STATE_RESULT_CONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB SEND RESULT CONNECT using ");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "GET" request
             */

            if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )
            {
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: Server:%s port: %d\n", cb_data->broker_server.host_name, cb_data->broker_server.port);
            }

            break;

        case CY_OTA_STATE_RESULT_SEND:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB SENDING RESULT using ");
            /* NOTE:

             *  HTTP - json_doc holds the HTTP "PUT"
             */

            if ( (cb_data->connection_type == CY_OTA_CONNECTION_HTTP) || (cb_data->connection_type == CY_OTA_CONNECTION_HTTPS) )

            {
                cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "HTTP: '%s' \n", cb_data->json_doc);
            }

            break;

        case CY_OTA_STATE_RESULT_RESPONSE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB Got Result response\n");

            break;

        case CY_OTA_STATE_RESULT_DISCONNECT:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB Result Disconnect\n");

            break;

        case CY_OTA_STATE_OTA_COMPLETE:
            cy_log_msg(CYLF_MIDDLEWARE, CY_LOG_INFO, "APP OTA CB Session Complete\n");
            break;

        case CY_OTA_NUM_STATES:
            break;

        default:
            break;

        }   /* switch state */
        break;
    }

    return cb_result;
}


/*******************************************************************************
* Function Name: sdio_interrupt_handler
********************************************************************************
* Summary:
* Interrupt handler function for SDIO instance.
*******************************************************************************/
void sdio_interrupt_handler(void)
{
    mtb_hal_sdio_process_interrupt(&sdio_instance);
}

/*******************************************************************************
* Function Name: app_sdio_init
********************************************************************************
* Summary:
* This function configures and initializes the SDIO instance used in
* communication between the host MCU and the wireless device.
*******************************************************************************/
void app_sdio_init(void)
{
    mtb_hal_sdio_cfg_t hal_cfg;

    /* Setup SDIO using the HAL object and desired configuration */
    mtb_hal_sdio_setup(&sdio_instance, &CYBSP_WIFI_SDIO_sdio_hal_config, NULL, &sdhc_host_context);

    /* Initialize and Enable SD HOST */
    Cy_SD_Host_Enable(sdio_instance.sdxx.base);
    Cy_SD_Host_Init(sdio_instance.sdxx.base, CYBSP_WIFI_SDIO_sdio_hal_config.host_config, &sdhc_host_context);
    Cy_SD_Host_SetHostBusWidth(sdio_instance.sdxx.base, CY_SD_HOST_BUS_WIDTH_4_BIT);

    cy_stc_sysint_t sdio_intr_cfg =
    {
        .intrSrc = CYBSP_WIFI_SDIO_IRQ,
        .intrPriority = APP_SDIO_INTERRUPT_PRIORITY
    };

    /* Initialize the SDIO interrupt and specify the interrupt handler. */
    cy_en_sysint_status_t interrupt_init_status = Cy_SysInt_Init(&sdio_intr_cfg, sdio_interrupt_handler);

    /* SDIO interrupt initialization failed. Stop program execution. */
    if(CY_SYSINT_SUCCESS != interrupt_init_status)
    {
        CY_ASSERT(0);
    }

    /* Enable NVIC interrupt. */
    NVIC_EnableIRQ(CYBSP_WIFI_SDIO_IRQ);

    hal_cfg.frequencyhal_hz =  APP_SDIO_FREQUENCY_HZ;
    hal_cfg.block_size = SDHC_SDIO_64B_BLOCK;

    /* Configure SDIO */
    mtb_hal_sdio_configure(&sdio_instance, &hal_cfg);

    /* Setup GPIO using the HAL object for WIFI WL REG ON  */
    mtb_hal_gpio_setup(&wifi_config.wifi_wl_pin, CYBSP_WIFI_WL_REG_ON_PORT_NUM, CYBSP_WIFI_WL_REG_ON_PIN);

    /* Setup GPIO using the HAL object for WIFI HOST WAKE PIN  */
    mtb_hal_gpio_setup(&wifi_config.wifi_host_wake_pin, CYBSP_WIFI_HOST_WAKE_PORT_NUM, CYBSP_WIFI_HOST_WAKE_PIN);
}



/* [] END OF FILE */
