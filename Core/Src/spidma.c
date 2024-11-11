/*
 * spidma.c
 *
 * Routines to have a queue of things to transfer
 * (outbound only) via SPI, with a command/data
 * and chip select pin.
 *
 * Functionality:
 * * Enqueue things
 *   * Provide an error when things cannot be enqueued
 *   * Queue entries have a generic 32-bit value for end user identification
 *   * Queue entries have a monotonically increasing internal identifier
 *     which is returned to the caller
 * * Get notification when a queued thing starts or stops
 * * Queue status queries
 *   * Counter of how many things are queued
 *   * What is currently being sent
 * * Automatic submission of queued items to SPI DMA
 *   * First setting the CS and DC signals
 *
 * Non-features:
 * * Thread safety
 *
 *  Created on: 2024-11-11
 *  Updated on: 2024-11-11
 *      Author: Douglas P. Fields, Jr.
 *   Copyright: 2024, Douglas P. Fields, Jr.
 *     License: Apache 2.0
 */


