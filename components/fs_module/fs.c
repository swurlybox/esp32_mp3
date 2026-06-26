#include "fs.h"

#include <stdio.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"


/* TODO: Hook up SPI pin numbers. Eventually make this a KConfig option
    to make this configurable via a separate module. */
#define SPI_MISO    12
#define SPI_MOSI    13
#define SPI_CLK     14
#define SPI_CS      15

#define MOUNT_POINT     "/sdcard"
#define MAX_OPEN_FILES  4    /* Arbitrary amount, not that many */
#define SECTOR_SIZE     512

/* Initialize SPI flash reader, SD card, and mount filesystem. */
void fs_init(void){
    esp_err_t ret;

    /* Mount config */
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = MAX_OPEN_FILES,
        .allocation_unit_size = SECTOR_SIZE,
        .disk_status_check_enable = true    /* docs says this affects perf */
    };

    /* Some card structure? */
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    
    /* Logging module? Don't use for now. */
    // ESP_LOGI
    printf("Initializing SD card\n");
    printf("Using SPI peripheral\n");

    /* Host description: SPI2 and max frequency set to 20MHz.
        From sdspi_host.h, a transitive include from esp_vfs_fat.h */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.unaligned_multi_block_rw_max_chunk_size = 8;
    host.max_freq_khz = 400;

    /* Prepare to initialize the SPI peripheral on specific pins. */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SPI_MOSI,
        .miso_io_num = SPI_MISO,
        .sclk_io_num = SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        printf("Failed to initialize SPI bus\n");
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SPI_CS;
    slot_config.host_id = host.slot;

    printf("Mounting filesystem\n");
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, 
                                    &mount_config, &card);
    host.max_freq_khz = 9000;   /* Ramp up frequency for faster I/O. */
    sdspi_host_set_card_clk(host.slot, 9000);   /* 9MHz seems to be the cap. */
    int real_khz = 0;
    sdspi_host_get_real_freq(host.slot, &real_khz);
    printf("SD Card I/O Real khz: %d\n", real_khz);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("Failed to mount filesystem.\n");
        } else {
            printf("Failed to initialize the card. Makes sure SD card lines"
            " have pull-up resistors in place.\n");
        }
        return;
    }
    printf("Filesystem mounted\n");

    /* Card has been initialized, print its properties */
    sdmmc_card_print_info(stdout, card);
}
