#include "fs.h"

#include <stdio.h>
#include <dirent.h>
#include <string.h>

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "input.h"
#include "display.h"

/* TODO: Hook up SPI pin numbers. Eventually make this a KConfig option
    to make this configurable via a separate module. */
#define SPI_MISO    12
#define SPI_MOSI    13
#define SPI_CLK     14
#define SPI_CS      15

#define MOUNT_POINT         "/sdcard"
#define MAX_OPEN_FILES      4    /* Arbitrary amount, not that many */
#define SECTOR_SIZE         512
#define MAX_CWD_PATH_LEN    512

/* DISPLAY OFFSETS */
#define CWD_MARGIN_OFFSET   (30)
#define FILETYPE_PX_OFFSET  (12)
#define FILENAME_PX_OFFSET  (24)

struct filesystem_context {
    uint32_t index;
    uint32_t dirent_count;
    char cwd[MAX_CWD_PATH_LEN];
} ctx;

/* Initialize SPI flash reader, SD card, and mount filesystem. */
int fs_init(void){
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
    host.max_freq_khz = 300;    /* TODO: varying voltages? */

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
        return -1;
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
        return -1;
    }
    printf("Filesystem mounted\n");

    /* Card has been initialized, print its properties */
    sdmmc_card_print_info(stdout, card);
    return 0;
}

/* Copied from my Zephyr NRF implementation */
static void chdir(const char *path) {
    DIR *dir;
    /* We use two character buffers here. Path is used in the relative pathing
        algorithm, utilizing strtok to prepend or append tokens to a final
        output cwd. */
    char temp_path[MAX_CWD_PATH_LEN] = {0}; // mutable copy of path
    char temp_cwd[MAX_CWD_PATH_LEN] = {0};  // mutable copy of cwd

    /* Absolute pathing: Detected by a starting '/'. Mount point is prepended,
        so user shouldn't worry about it. */
    if (path[0] == '/') {
        strcat(temp_cwd, MOUNT_POINT);
        strcat(temp_cwd, path);
        dir = opendir(temp_cwd);
        if (!dir) {
            printf("Error changing dir %s\n", path);
            return;
        }

        printf("Dir exists, changing cwd to %s\n", path);
        strncpy(ctx.cwd, path, MAX_CWD_PATH_LEN);
        ctx.index = 0;
        closedir(dir);
        return;
    }

    /* Relative pathing: 99% of the time we will be using this route. 
        .. backtracks to a parent directory
        . is ignored
        The approach here is to mutate temp_path as we traverse the tokens
        in the provided user's path string. If a directory doesn't exist, 
        we fail and the original cwd will stay intact. If at the end we find
        a valid directory, we'll set the cwd as temp_path. */
    strncpy(temp_cwd, ctx.cwd, MAX_CWD_PATH_LEN);
    strncpy(temp_path, path, MAX_CWD_PATH_LEN);
    char *saveptr;
    char *token;
    char *c_to_rmv;
    token = strtok_r(temp_path, "/", &saveptr);
    while(token != NULL) {
        /* Backtrack to parent directory */
        if (strcmp(token, "..") == 0) {
            /* A cwd of "/" or just '\0', will still refer to root directory.*/
            if ((c_to_rmv = strrchr(temp_cwd, '/')) != NULL) {
                *c_to_rmv = '\0';
            }
        }
        else if (strcmp(token, ".") == 0) {
            /* Do nothing. */
        } else {
            /* Append the token to the current path. */
            strcat(temp_cwd, "/");
            strcat(temp_cwd, token);
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    /* After fully parsing the user's relative path, we should end up with
        a contemporary current working directory that's relative to the mount
        point. */
    *temp_path = '\0'; /* We'll reuse temp_path to prepend the mount point. */
    strcat(temp_path, MOUNT_POINT);
    strcat(temp_path, temp_cwd);
    dir = opendir(temp_path);
    if (!dir) {
        printf("Error changing dir %s\n", temp_cwd);
        return;
    }

    printf("Dir exists, changing cwd to %s\n", temp_cwd);
    strncpy(ctx.cwd, temp_cwd, MAX_CWD_PATH_LEN);
    ctx.index = 0;
    closedir(dir);
    return;
}

#define WIN_DEF_START   (0)
#define WIN_DEF_END     (6)

/* Automatically prepends mount point, so caller just focuses on the path. */
static void lsdir(const char *path) {
    char temp_path[MAX_CWD_PATH_LEN] = {0};
    strcat(temp_path, MOUNT_POINT);
    strcat(temp_path, path);

    DIR *dir = opendir(temp_path);
    if (!dir) {
        printf("Failed to open directory: %s\n", path);
        return;
    }

    /* UART output. */
    printf("%s:\n", temp_path);
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        
        if (count == ctx.index) {
            printf("> ");
        } else {
            printf("  ");
        }

        printf(
            "    %s: %s\n",
            (entry->d_type == DT_DIR)
                ? "directory"
                : "file     ",
            entry->d_name
        );
        count++;
    }

    ctx.dirent_count = count;

    /* We need the dirent count before we can do the display output. Hence,
        why these two sections are separated. */
    /* DISPLAY output */
    uint8_t row = 1;
    uint8_t win_start;
    uint8_t win_end;

    graphics_clear();
    graphics_draw_line_chars("CWD: ", 0, 0, 5);
    graphics_draw_line_chars(ctx.cwd, 0, CWD_MARGIN_OFFSET, 20);

    if (ctx.dirent_count <= 7 || ctx.index <= 2) {
        win_start   = WIN_DEF_START;
        win_end     = WIN_DEF_END;
    }
    else if (ctx.dirent_count - ctx.index <= 3) {
        win_start   = (uint8_t) (ctx.dirent_count - 7U);
        win_end     = (uint8_t) (ctx.dirent_count - 1U);   
    }
    else {
        win_start   = (uint8_t) (ctx.index - 3U);
        win_end     = (uint8_t) (ctx.index + 3U);
    }

    uint8_t index = 0;
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        if ((index) >= win_start && (index) <= win_end) {
            if (index == ctx.index) {
                graphics_draw_line_chars(">", row, 0, 1);
            }
            if (entry->d_type == DT_DIR) {
                graphics_draw_line_chars("D", row, FILETYPE_PX_OFFSET, 1);
            }
            else {
                graphics_draw_line_chars("F", row, FILETYPE_PX_OFFSET, 1);
            }
            graphics_draw_line_chars(entry->d_name, row, FILENAME_PX_OFFSET, 
                20);
            row++;
        }
        index++;
    }
    xSemaphoreGive(display_semaphore);
    closedir(dir);
}

static void fs_up(void) {
    if (ctx.index == 0) {
        ctx.index = ctx.dirent_count - 1;
    } else {
        ctx.index--;
    }
    lsdir(ctx.cwd);
};

static void fs_down(void) { 
    if (ctx.index == ctx.dirent_count - 1) {
        ctx.index = 0;
    } else {
        ctx.index++;
    }
    lsdir(ctx.cwd);
};

/* TODO: Implement rest of file system navigation functions */
static void fs_select(void) {
    DIR* dir;
    int count = 0;
    char temp_path[MAX_CWD_PATH_LEN] = {0};
    
    strcat(temp_path, MOUNT_POINT);
    strcat(temp_path, ctx.cwd);

    dir = opendir(temp_path);
    if (!dir) {
        printf("Error opening dir %s\n", ctx.cwd);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (count == ctx.index) {
            break;
        }
        count++;
    }

    if (entry == NULL) {
        printf("Couldn't find entry corresponding to index: malformed ctx");
        return;
    }

    if (entry->d_type == DT_DIR) {
        chdir(entry->d_name);
        lsdir(ctx.cwd);
    } else {
        printf("FILE: %s\n", entry->d_name);
    }
}

static void fs_cancel(void) {
    chdir("..");
    ctx.index = 0;
    ctx.dirent_count = 0;
    lsdir(ctx.cwd);
}

/* TODO: Filesystem navigation */
void fs_enter() {
    button_cb_reset_all();
    button_cb_register(UP_BUTTON, fs_up);
    button_cb_register(DOWN_BUTTON, fs_down);
    button_cb_register(SELECT_BUTTON, fs_select);
    button_cb_register(CANCEL_BUTTON, fs_cancel);

    lsdir(ctx.cwd);
}























