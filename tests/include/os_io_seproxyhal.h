#pragma once

/**
 * Function to ensure a I/O channel is not timeouting waiting for operations
 * after a long time without SEPH packet exchanges
 */
void io_seproxyhal_io_heartbeat(void);