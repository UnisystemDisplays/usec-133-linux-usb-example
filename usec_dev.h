#ifndef __USEC_DEV_H_
#define __USEC_DEV_H_

#include <stdint.h>
#include <stddef.h>

/******************************************************************************/

#define USEC_DEV_OK             (0)
#define USEC_DEV_ERR            (1)

/* enable/disable logs */
#define USEC_DEV_DEBUG_LOG      (0)

/* do not modify */
#define USEC_DEV_FAST_WRITE     (1)
#define USEC_DEV_SENSE_LEN      (256)
#define USEC_DEV_BLOCK_LEN      (32)
#define USEC_DEV_TIMEOUT        (50000)
#define USEC_DEV_SPT_LEN        (60*1024)

/******************************************************************************/

/*
 * Update modes:
 * (source: https://github.com/m5stack/M5EPD/blob/main/src/M5EPD_Driver.h)
 *
 * UPDATE_MODE_INIT - is used to completely erase the display and leave it in
 * the white state. It is useful for situations where the display information in
 * memory is not a faithful representation of the optical state of the display,
 * for example, after the device receives power after it has been fully powered
 * down.
 *
 * UPDATE_MODE_DU - is a very fast, non-flashy update. This mode supports
 * transitions from any graytone to black or white only. It cannot be used to
 * update to any graytone other than black or white.
 *
 * UPDATE_MODE_GC16 - is used to update the full display and provide a high
 * image quality. When GC16 is used with Full Display Update the entire display
 * will update as the new image is written. If a Partial Update command is used
 * the only pixels with changing graytone values will update. The GC16 mode has
 * 16 unique gray levels.
 *
 * UPDATE_MODE_GL16 - is primarily used to update sparse content on a white
 * background, such as a page of anti-aliased text, with reduced flash. The GL16
 * waveform has 16 unique gray levels.
 *
 * UPDATE_MODE_GLR16 - is used in conjunction with an image preprocessing
 * algorithm to update sparse content on a white background with reduced flash
 * and reduced image artifacts. The GLR16 mode supports 16 graytones.
 *
 * UPDATE_MODE_GLD16 - is used in conjunction with an image preprocessing
 * algorithm to update sparse content on a white background with reduced flash
 * and reduced image artifacts. It is recommended to be used only with the full
 * display update. The GLD16 mode supports 16 graytones.
 *
 * UPDATE_MODE_A2 - is a fast, non-flash update mode designed for fast paging
 * turning or simple black/white animation. This mode supports transitions from
 * and to black or white only. It cannot be used to update to any graytone other
 * than black or white.
 *
 * UPDATE_MODE_DU4 - is a fast update time (similar to UPDATE_MODE_DU),
 * non-flashy waveform. This mode supports gray-gray transitions.
 */

enum
{
  UPDATE_MODE_INIT,
  UPDATE_MODE_DU,
  UPDATE_MODE_GC16,
  UPDATE_MODE_GL16,
  UPDATE_MODE_GLR16,
  UPDATE_MODE_GLD16,
  UPDATE_MODE_A2,
  UPDATE_MODE_DU4
};

/******************************************************************************/

enum
{
  IMG_1BPP,
  IMG_2BPP,
  IMG_4BPP,
  IMG_8BPP
};

/******************************************************************************/

typedef struct
{
  int        dev_fd[1];     /* device file descriptor */
  uint32_t   dev_width[1];  /* screen width [px]  */
  uint32_t   dev_height[1]; /* screen height [px] */
  uint32_t   dev_addr[1];   /* only for internal usage */
  uint8_t   *dev_sense_buf; /* only for internal usage */
} usec_ctx;

/******************************************************************************/

usec_ctx *
usec_init                    (const uint8_t  *dev_path);

void
usec_deinit                  (usec_ctx       *ctx);

uint8_t
usec_get_temp                (usec_ctx       *ctx,
                              uint8_t        *temp_val);

uint8_t
usec_get_vcom                (usec_ctx       *ctx,
                              uint16_t       *vcom_val);

uint8_t
usec_img_upload              (usec_ctx       *ctx,
                              uint8_t        *img_data,
                              size_t          img_size,
                              uint8_t         img_bpp,
                              uint32_t        img_pos_x,
                              uint32_t        img_pos_y,
                              uint32_t        img_width,
                              uint32_t        img_height);

uint8_t
usec_img_update              (usec_ctx       *ctx,
                              uint32_t        area_pos_x,
                              uint32_t        area_pos_y,
                              uint32_t        area_width,
                              uint32_t        area_height,
                              uint8_t         update_mode,
                              uint8_t         update_wait);

/******************************************************************************/

#endif /* __USEC_DEV_H_ */
