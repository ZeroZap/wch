/********************************** (C) COPYRIGHT *******************************
 * File Name          : img_install.h
 * Description        : Receives, decrypts, verifies and stores a signed image.
 *
 * The image arrives as ciphertext in 56 byte chunks.  Bytes are accumulated
 * into a 256 byte flash page, decrypted with AES-128-CTR when the encrypted
 * flag is set, hashed (SHA-256 + CRC-32 of the plaintext, HMAC of the stored
 * bytes) and only written once the whole page is complete.
 *
 *   BEGIN  -> validate the header fields, wipe header/flag page
 *   DATA*  -> stream
 *   END    -> compare the HMAC, then write the image header and the flag
 *
 * A failed or interrupted transfer leaves no valid flag behind, so the next
 * reset stays in the bootloader instead of jumping into a half written app.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#ifndef __IMG_INSTALL_H
#define __IMG_INSTALL_H

#include <stdint.h>
#include "img_hdr.h"

/* Start a transfer.  Returns IAP_ST_OK or an IAP_ST_* error. */
uint8_t img_install_begin(uint32_t img_len, uint16_t flags, uint32_t load_addr,
                          const uint8_t iv[16]);

/* Feed received (still encrypted) bytes.  Returns IAP_ST_OK / IAP_ST_* . */
uint8_t img_install_data(const uint8_t *data, uint32_t len);

/* Finish the transfer.  hmac_in is the 32 byte tag computed by the host.
 * On IAP_ST_OK the header and flag pages have been written. */
uint8_t img_install_finish(const uint8_t hmac_in[32]);

/* Give up and wipe the flag page. */
void    img_install_abort(void);

/* Re-read the stored image and verify it.  Returns IAP_ST_OK when the app in
 * the slot is intact and the flag page says IMG_STATE_VALID. */
uint8_t img_verify_stored(void);

/* 1 while a transfer is in progress, so the bootloader knows not to jump. */
uint8_t img_install_active(void);

/* Load the stored header into *out.  Returns 1 on a plausible header. */
uint8_t img_load_hdr(img_hdr_t *out);

/* Load the flag page.  Returns 1 when magic and state are sane. */
uint8_t img_load_flag(img_flag_t *out);

/* Persist a flag record (used to mark the app valid/invalid). */
uint8_t img_write_flag(uint32_t state, const img_hdr_t *hdr);

#endif /* __IMG_INSTALL_H */
