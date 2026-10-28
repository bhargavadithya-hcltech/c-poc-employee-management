/**
 * @file main.c
 * @brief Program entry point. All logic lives in the modules.
 */
#include "app.h"

int main(void)
{
    return runApplication(DEFAULT_DATA_DIR, DEFAULT_LOG_FILE);
}
