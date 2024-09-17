#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include "usec_dev.h"

/* stb_image */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

/* definitions */
#define DEMO_OK     0
#define DEMO_ERR    1

/* prototypes */
static uint8_t screen_cleanup (usec_ctx *ctx);
static uint8_t demo_1_image_fullscreen (usec_ctx *ctx);
static uint8_t demo_2_image_partial (usec_ctx *ctx);
static uint8_t demo_3_image_non_flash (usec_ctx *ctx);
static uint8_t demo_4_raw_fullscreen (usec_ctx *ctx);

/******************************************************************************/

/*
 * main()
 */
int
main (void)
{
  usec_ctx *ctx;
  uint8_t status;
  uint8_t usec_temp;
  uint16_t usec_vcom;

  /* initialize controller */
  ctx = usec_init (NULL);
  if (ctx == NULL)
    {
      printf ("[error] cannot initialize e-ink controller\n\r");
      return EXIT_FAILURE;
    }

  /* read temperature value */
  status = usec_get_temp (ctx, &usec_temp);
  if (status != USEC_DEV_OK)
    {
      printf ("[error] cannot read temperature value\n\r");

      usec_deinit (ctx);
      return EXIT_FAILURE;
    }
  printf ("[status] screen temperature: %d [degC]\n\r", usec_temp);

  /* read vcom value */
  status = usec_get_vcom (ctx, &usec_vcom);
  if (status != USEC_DEV_OK)
    {
      printf ("[error] cannot read VCOM value\n\r");

      usec_deinit (ctx);
      return EXIT_FAILURE;
    }
  printf ("[status] screen VCOM: -%.2f [V]\n\r", (float) usec_vcom / 1000);

  /* cleanup screen - fullscreen update with UPDATE_MODE_INIT mode
   * (do not modify internal buffer content) */
  screen_cleanup (ctx);

  /* [DEMO 1]
   * Fullscreen update with UPDATE_MODE_GC16 mode
   *
   * UPDATE_MODE_GC16 - is used to update the full display and provide a high
   * image quality. When GC16 is used with Full Display Update the entire
   * display will update as the new image is written. If a Partial Update
   * command is used the only pixels with changing graytone values will update.
   * The GC16 mode has 16 unique gray levels */

  demo_1_image_fullscreen (ctx);
  sleep (2);
  screen_cleanup (ctx);

  /* [DEMO 2]
   * Parital update with UPDATE_MODE_GC16 mode
   *
   * UPDATE_MODE_GC16 - is used to update the full display and provide a high
   * image quality. When GC16 is used with Full Display Update the entire
   * display will update as the new image is written. If a Partial Update
   * command is used the only pixels with changing graytone values will update.
   * The GC16 mode has 16 unique gray levels */

  demo_2_image_partial (ctx);
  sleep (2);

  /* [DEMO 3]
   * Parital, non-flash update with UPDATE_MODE_A2 mode
   *
   * UPDATE_MODE_A2 - is a fast, non-flash update mode designed for fast paging
   * turning or simple black/white animation. This mode supports transitions
   * from and to black or white only. It cannot be used to update to any
   * graytone other than black or white */

  demo_3_image_non_flash (ctx);
  sleep (2);

  /* [DEMO 4]
   * Fullscreen update with UPDATE_MODE_GC16 mode directly from RAW file
   */

  demo_4_raw_fullscreen (ctx);

  /* cleanup */
  usec_deinit (ctx);
  ctx = NULL;

  return EXIT_SUCCESS;
}

/******************************************************************************/

/*
 * screen_cleanup()
 */
static uint8_t
screen_cleanup (usec_ctx *ctx)
{
  return usec_img_update (ctx, 0, 0, 1600, 1200, UPDATE_MODE_INIT, 0);
}

/******************************************************************************/

/*
 * demo_1_image_fullscreen()
 */
static uint8_t
demo_1_image_fullscreen (usec_ctx *ctx)
{
  uint8_t *img_data;
  uint8_t status;
  int img_res_x, img_res_y, img_ch_num;

  /* load BMP image - convert to grayscale (1 channel) */
  img_data = stbi_load ("images/img_1.bmp", &img_res_x, &img_res_y,
			                  (int *) &img_ch_num, 1);
  if (img_data == NULL)
    {
      printf ("[error] cannot load 'images/img_1.bmp' image\n\r");
      return DEMO_ERR;
    }

  /* show image info */
  printf ("[demo_1] input image resolution: %dx%d\n\r", img_res_x, img_res_y);

  /* upload and display image */
  printf ("[demo_1] uploading 'images/img_1.bmp' image\n\r");
  status = usec_img_upload (ctx, img_data, img_res_x * img_res_y * 1,
		                        IMG_8BPP, 0, 0, 1600, 1200);
  if (status != USEC_DEV_OK)
    {
      printf ("[error] cannot upload 'images/img_1.bmp' image\n\r");
      return DEMO_ERR;
    }

  /* fullscreen update with UPDATE_MODE_GC16 */
  printf ("[demo_1] displaying 'images/img_1.bmp' image\n\r");
  status = usec_img_update (ctx, 0, 0, 1600, 1200, UPDATE_MODE_GC16, 0);
  if (status != USEC_DEV_OK)
    {
      printf ("[error] cannot display 'images/img_1.bmp' image\n\r");
      return DEMO_ERR;
    }

  /* cleanup */
  stbi_image_free (img_data);
  return DEMO_OK;
}

/******************************************************************************/

/*
 * demo_2_image_partial()
 */
static uint8_t
demo_2_image_partial (usec_ctx *ctx)
{
  for (uint8_t cnt = 0; cnt < 4; cnt++)
    {
      char *img_path;
      uint8_t *img_data, status;
      uint32_t pos_x, pos_y;
      int img_res_x, img_res_y, img_ch_num;

      switch (cnt)
        {
          case 0:
            pos_x = 200; pos_y = 50; img_path = "images/img_2.bmp";
          break;
          case 1:
            pos_x = 900; pos_y = 50; img_path = "images/img_3.bmp";
          break;
          case 2:
            pos_x = 200; pos_y = 650; img_path = "images/img_4.bmp";
          break;
          case 3:
            pos_x = 900; pos_y = 650; img_path = "images/img_5.bmp";
          break;
        }

      /* load image - convert to grayscale (1 channel) */
      img_data = stbi_load (img_path, &img_res_x, &img_res_y,
			                      (int *) &img_ch_num, 1);
      if (img_data == NULL)
	      {
	        printf ("[error] cannot load '%s' image\n\r", img_path);
	        return DEMO_ERR;
        }

      /* show image info */
      printf ("[demo_2] input image resolution: %dx%d\n\r", img_res_x,
              img_res_y);

      /* upload and display image */
      printf ("[demo_2] uploading '%s' image\n\r", img_path);
      status = usec_img_upload (ctx, img_data, img_res_x * img_res_y * 1,
			                          IMG_8BPP, pos_x, pos_y, img_res_x, img_res_y);
      if (status != USEC_DEV_OK)
	      {
	        printf ("[error] cannot upload '%s' image\n\r", img_path);
	        return DEMO_ERR;
	      }

      /* fullscreen update with UPDATE_MODE_GC16 */
      printf ("[demo_2] displaying '%s' image\n\r", img_path);
      status = usec_img_update (ctx, pos_x, pos_y,
                                img_res_x, img_res_y, UPDATE_MODE_GC16, 0);
      if (status != USEC_DEV_OK)
	      {
	        printf ("[error] cannot display '%s' image\n\r", img_path);
	        return DEMO_ERR;
        }

      /* cleanup */
      stbi_image_free (img_data);
    }

  return DEMO_OK;
}

/******************************************************************************/

/*
 * demo_3_image_non_flash()
 */
static uint8_t
demo_3_image_non_flash (usec_ctx *ctx)
{
  /* clear internal image buffer via uploading white (0xff) image */
  {
    uint8_t tmp_buf[1600*1200];
    uint8_t status;

    memset (tmp_buf, 0xff, sizeof(tmp_buf));

    /* upload and display image */
    status = usec_img_upload (ctx, tmp_buf, sizeof(tmp_buf),
		                          IMG_8BPP, 0, 0, 1600, 1200);
    if (status != USEC_DEV_OK)
      {
        printf ("[error] cannot upload 'images/img_1.bmp' image\n\r");
        return DEMO_ERR;
      }

    status = usec_img_update (ctx, 0, 0, 1600, 1200, UPDATE_MODE_GC16, 0);
    if (status != USEC_DEV_OK)
      {
        printf ("[error] cannot display 'images/img_1.bmp' image\n\r");
        return DEMO_ERR;
      }
  }

  for (uint8_t cnt = 0; cnt < 3; cnt++)
    {
      char *img_path;
      uint8_t *img_data, status;
      uint32_t pos_x, pos_y;
      int img_res_x, img_res_y, img_ch_num;

      switch (cnt)
        {
          case 0:
            pos_x = 200; pos_y = 150; img_path = "images/img_6.bmp";
          break;
          case 1:
            pos_x = 200; pos_y = 500; img_path = "images/img_7.bmp";
          break;
          case 2:
            pos_x = 200; pos_y = 850; img_path = "images/img_8.bmp";
          break;
        }

      /* load image */
      img_data = stbi_load (img_path, &img_res_x, &img_res_y,
			                      (int *) &img_ch_num, 1);
      if (img_data == NULL)
	      {
	        printf ("[error] cannot load '%s' image\n\r", img_path);
	        return DEMO_ERR;
        }

      /* show image info */
      printf ("[demo_3] input image resolution: %dx%d\n\r", img_res_x,
              img_res_y);

      /* upload and display image */
      printf ("[demo_3] uploading '%s' image\n\r", img_path);
      status = usec_img_upload (ctx, img_data, img_res_x * img_res_y * 1,
			                          IMG_8BPP, pos_x, pos_y, img_res_x, img_res_y);
      if (status != USEC_DEV_OK)
	      {
	        printf ("[error] cannot upload '%s' image\n\r", img_path);
	        return DEMO_ERR;
	      }

      /* fullscreen update with UPDATE_MODE_A2 */
      printf ("[demo_3] displaying '%s' image\n\r", img_path);
      status = usec_img_update (ctx, pos_x, pos_y,
                                img_res_x, img_res_y, 4, 0);
      if (status != USEC_DEV_OK)
	      {
	        printf ("[error] cannot display '%s' image\n\r", img_path);
	        return DEMO_ERR;
        }

      /* cleanup */
      stbi_image_free (img_data);
    }

  return DEMO_OK;
}

/******************************************************************************/

/*
 * demo_4_raw_fullscreen()
 */
static uint8_t
demo_4_raw_fullscreen (usec_ctx *ctx)
{
  /* TODO */

  return DEMO_OK;
}
