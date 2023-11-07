/* *****************************************************************************
 * File:   cmd_system.c
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
#include "cmd_system.h"

#include <string.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_console.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

#include "esp_chip_info.h"

#include "esp_flash.h"
#if defined(USE_5_0_RELEASE) && (USE_5_0_RELEASE)
#include "spi_flash_mmap.h"
#else
#include "esp_spi_flash.h" //esp_spi_flash.h is (USE_5_0_RELEASE) deprecated, please use spi_flash_mmap.h instead
#endif

#include "esp_mac.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "argtable3/argtable3.h"
#include "drv_version_if.h"
#include "drv_system_if.h"

#include "drv_console_if.h"

/* *****************************************************************************
 * Configuration Definitions
 **************************************************************************** */
#define TAG "cmd_system"

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

/* *****************************************************************************
 * Prototype of functions definitions
 ****************************************************************************udp_stream_recvfree_mem */

/* *****************************************************************************
 * Functions
 **************************************************************************** */

/* 'version' command */
static int get_chip(int argc, char **argv)
{
    esp_chip_info_t info;
    esp_chip_info(&info);
    printf("IDF Version:");
    printf("%s", esp_get_idf_version());
    printf("\n\r");
    printf("Chip info:\n\r");
    printf("\tmodel:");
    printf("%s", info.model == CHIP_ESP32 ? "ESP32" : 
                 info.model == CHIP_ESP32S2 ? "ESP32S2" : 
                 info.model == CHIP_ESP32S3 ? "ESP32S3" : 
                 info.model == CHIP_ESP32C3 ? "ESP32C3" : 
                 #if USE_5_0_RELEASE
                 info.model == CHIP_ESP32C2 ? "ESP32C2" : 
                 #endif
                 info.model == CHIP_ESP32H2 ? "ESP32H2" : "Unknow");
    printf("\n\r");
    printf("\tcores:");
    printf("%d", info.cores);
    printf("\n\r");
    printf("\tfeature:");
    printf("%s", info.features & CHIP_FEATURE_WIFI_BGN ? "/802.11bgn" : "");
    printf("%s", info.features & CHIP_FEATURE_BLE ? "/BLE" : "");
    printf("%s", info.features & CHIP_FEATURE_BT ? "/BT" : "");
    printf("%s", info.features & CHIP_FEATURE_EMB_FLASH ? "/Embedded-Flash:" : "/External-Flash:");
    printf("%s", info.features & CHIP_FEATURE_IEEE802154 ? "/IEEE 802.15.4:" : "");
    #ifdef CHIP_FEATURE_EMB_PSRAM
    printf("%s", info.features & CHIP_FEATURE_EMB_PSRAM ? "/Embedded-PSRAM" : "");
    #endif

    uint32_t size_flash_chip;
	#if defined(USE_5_0_RELEASE) && (USE_5_0_RELEASE)
    esp_flash_get_size(NULL, &size_flash_chip);
	#else
    size_flash_chip = (uint32_t)spi_flash_get_chip_size();
    #endif
    printf("%d", (int)size_flash_chip / (1024 * 1024));
    printf("%s", " MB");
    printf("\n\r");

    printf("\trevision number:");
    printf("%d", info.revision);
    printf("\n\r");
    return 0;
}




static void register_chip(void)
{
    const esp_console_cmd_t cmd_version = {
        .command = "chip",
        .help = "Get chip info and SDK",
        .hint = NULL,
        .func = &get_chip,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_version) );
}


static int tasks_info(int argc, char **argv)
{
    const size_t bytes_per_task = 40; /* see vTaskList description */
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
        return 1;
    }
    fputs("Task Name\tStatus\tPrio\tHWM\tNo", stdout);
    #ifdef CONFIG_FREERTOS_VTASKLIST_INCLUDE_COREID
    fputs("\tAffinity", stdout);
    #endif
    fputs("\n\r", stdout);
    vTaskList(task_list_buffer);
    fputs(task_list_buffer, stdout);
    free(task_list_buffer);
    return 0;
}

static void register_tasks(void)
{
    const esp_console_cmd_t cmd_tasks = {
        .command = "tasks",
        .help = "Get information about running tasks",
        .hint = NULL,
        .func = &tasks_info,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_tasks) );
}

#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
static int load_info(int argc, char **argv)
{
    const size_t bytes_per_task = 40; /* see vTaskList description */
    char *task_list_buffer = malloc(uxTaskGetNumberOfTasks() * bytes_per_task);
    if (task_list_buffer == NULL) {
        ESP_LOGE(TAG, "failed to allocate buffer for vTaskList output");
        return 1;
    }
    fputs("\n\r", stdout);
    fputs("\n\r", stdout);
    fputs("\n\r", stdout);
    fputs("Task Name\tTimerTicks\tCPU Load#", stdout);
    fputs("\n\r", stdout);
    vTaskGetRunTimeStats(task_list_buffer);
    fputs(task_list_buffer, stdout);
    free(task_list_buffer);
    return 0;
}

static void register_load(void)
{
    const esp_console_cmd_t cmd_load = {
        .command = "load",
        .help = "Get information about running tasks CPU load",
        .hint = NULL,
        .func = &load_info,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_load) );
}
#endif

void cmd_system_tasks_info(void)
{
    tasks_info((int)NULL, (char**)NULL);
}



static int mem_info(int argc, char **argv)
{  
    drv_console_set_log_disabled();


    multi_heap_info_t info;

    printf("|%16s|%16s|%16s|%16s|%16s|\r\n", "memory            ", "free", "allocated", "min_free", "max_free_block");
    heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_DEFAULT       ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_INTERNAL      ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_IRAM_8BIT);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_IRAM_8BIT     ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_8BIT);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_8BIT          ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_32BIT);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_32BIT         ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_RTCRAM);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_RTCRAM        ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_DMA);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_DMA           ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_EXEC);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_EXEC          ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);
    heap_caps_get_info(&info, MALLOC_CAP_RETENTION);
    printf("|%16s|%16d|%16d|%16d|%16d|\r\n", "CAP_RETENTION     ", info.total_free_bytes, info.total_allocated_bytes, info.minimum_free_bytes, info.largest_free_block);


    return 0;

}

static void register_mem(void)
{
    const esp_console_cmd_t cmd_heap = {
        .command = "mem",
        .help = "Get memory information during program execution",
        .hint = NULL,
        .func = &mem_info,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_heap));
}


static int heap_size(int argc, char **argv)
{  
    drv_console_set_log_disabled();

    printf("min heap (default       ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    printf("\r\n");

    printf("min heap (internal only ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    printf("\r\n");
    printf("min heap (IRAM 8BIT     ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_IRAM_8BIT));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_IRAM_8BIT));
    printf("\r\n");
    printf("min heap (8BIT          ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    printf("\r\n");
    printf("min heap (32BIT         ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_32BIT));
    printf("\r\n");
    printf("min heap (RTCRAM        ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_RTCRAM));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_RTCRAM));
    printf("\r\n");
    printf("min heap (DMA           ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_DMA));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_DMA));
    printf("\r\n");
    printf("min heap (EXEC          ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_EXEC));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_EXEC));
    printf("\r\n");
    printf("min heap (RETENTION     ): ");
    printf("%6d", heap_caps_get_minimum_free_size(MALLOC_CAP_RETENTION));
    printf("|%6d", heap_caps_get_largest_free_block(MALLOC_CAP_RETENTION));
    printf("\r\n");

    return 0;

}

static void register_heap(void)
{
    const esp_console_cmd_t cmd_heap = {
        .command = "heap",
        .help = "Get minimum size of free heap memory during program execution",
        .hint = NULL,
        .func = &heap_size,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_heap));
}


static int free_mem(int argc, char **argv)
{
    drv_console_set_log_disabled();

    printf("free mem (default       ): ");
    printf("%" PRIu32, esp_get_free_heap_size());
    printf("\r\n");

    printf("free mem (internal only ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    printf("\r\n");
    printf("free mem (IRAM 8BIT     ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_IRAM_8BIT));
    printf("\r\n");
    printf("free mem (8BIT          ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("\r\n");
    printf("free mem (32BIT         ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_32BIT));
    printf("\r\n");
    printf("free mem (RTCRAM        ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_RTCRAM));
    printf("\r\n");
    printf("free mem (DMA           ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_DMA));
    printf("\r\n");
    printf("free mem (EXEC          ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_EXEC));
    printf("\r\n");
    printf("free mem (RETENTION     ): ");
    printf("%d", heap_caps_get_free_size(MALLOC_CAP_RETENTION));
    printf("\r\n");

    printf("free mem (internal + dma): ");
    printf("%" PRIu32, esp_get_free_internal_heap_size());
    printf("\r\n");


    return 0; 
}

static void register_free(void)
{
    const esp_console_cmd_t cmd_free = {
        .command = "free",
        .help = "Get current size of free memory",
        .hint = NULL,
        .func = &free_mem,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_free));
}

static int restart(int argc, char **argv)
{
    ESP_LOGI(TAG, "Restarting...");
    esp_restart();
    return 0;
}

static void register_reset(void)
{
    const esp_console_cmd_t cmd_reset = {
        .command = "reset",
        .help = "Software reset of the chip",
        .hint = NULL,
        .func = &restart,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_reset));
}





static struct {
    struct arg_str *command;
    struct arg_end *end;
} man_args;


static int man(int argc, char **argv)
{
    drv_console_set_other_log_disabled();

    ESP_LOGI(__func__, "argc=%d", argc);
    for (int i = 0; i < argc; i++)
    {
        ESP_LOGI(__func__, "argv[%d]=%s", i, argv[i]);
    }

    int nerrors = arg_parse(argc, argv, (void **)&man_args);

    if (nerrors != ESP_OK)
    {
        arg_print_errors(stderr, man_args.end, argv[0]);
        return ESP_FAIL;
    }

    char cTemp[64];
    #define MACSTR_U "%02X:%02X:%02X:%02X:%02X:%02X"
    uint8_t mac_addr[6] = {0};
    uint8_t* mac_addr_last = drv_system_get_last_mac_identification_request();
    
    if (memcmp(mac_addr, mac_addr_last, 6) == 0)
    {
        esp_base_mac_addr_get(mac_addr);
    }
    else
    {
        memcpy(mac_addr, mac_addr_last, 6);
    }

    if (strcmp(man_args.command->sval[0],"mac") == 0)
    { 

        sprintf(cTemp, MACSTR_U "\r\nMAC:" MACSTR_U "\r\n", 
                    MAC2STR(mac_addr), MAC2STR(mac_addr));
        printf("%s", cTemp);
    }    
    else
    if (strcmp(man_args.command->sval[0],"ver") == 0)
    {       
        sprintf(cTemp, "Version:%d.%d.%05d\r\n", 
                    DRV_VERSION_MAJOR, DRV_VERSION_MINOR, DRV_VERSION_BUILD);
        printf("%s", cTemp);
    }    
    else
    {      
        sprintf(cTemp, MACSTR_U "\r\nMAC:" MACSTR_U "\r\nVersion:%d.%d.%05d\r\n", 
                    MAC2STR(mac_addr), MAC2STR(mac_addr), 
                    DRV_VERSION_MAJOR, DRV_VERSION_MINOR, DRV_VERSION_BUILD);
        printf("%s", cTemp);
    }
    memset(mac_addr, 0, 6);
    drv_system_set_last_mac_identification_request(mac_addr);

    return 0;
}

static void register_man(void)
{

    man_args.command = arg_strn(NULL, NULL, "<command>", 0, 1, "Command can be : man [mac|ver]");
    man_args.end = arg_end(1);

    const esp_console_cmd_t cmd_man = {
        .command = "man",
        .help = "Management Report MAC and Version. Command can be :man [mac|ver]",
        .hint = NULL,
        .func = &man,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_man));
}

void register_system_common(void)
{
    register_heap();
    register_free();
    register_reset();
    register_mem();
    register_man();
    register_tasks();
    #if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    register_load();
    #endif
    register_chip();
}

void cmd_system_register(void)
{
    register_system_common();
}