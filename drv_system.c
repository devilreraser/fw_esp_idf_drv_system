/* *****************************************************************************
 * File:   drv_system.c
 * Author: Dimitar Lilov
 *
 * Created on 2022 06 18
 * 
 * Description: ...
 * 
 **************************************************************************** */

/* *****************************************************************************
 * Header Includes
 **************************************************************************** */
#include "drv_system.h"

#include <string.h>

/* *****************************************************************************
 * Configuration Definitions
 **************************************************************************** */
#define TAG "drv_system"

/* *****************************************************************************
 * Constants and Macros Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Enumeration Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Type Definitions
 **************************************************************************** */

/* *****************************************************************************
 * Function-Like Macros
 **************************************************************************** */

/* *****************************************************************************
 * Variables Definitions
 **************************************************************************** */
uint8_t last_mac_identification_request[6] = {0};
/* *****************************************************************************
 * Prototype of functions definitions
 **************************************************************************** */

/* *****************************************************************************
 * Functions
 **************************************************************************** */

void drv_system_set_last_mac_identification_request(uint8_t* mac_address)
{
    memcpy(last_mac_identification_request, mac_address, 6);
}

uint8_t* drv_system_get_last_mac_identification_request(void)
{
    return last_mac_identification_request;
}
