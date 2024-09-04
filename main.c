#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "usec_dev.h"

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
  printf ("[log] screen temperature: %d [degC]\n\r", usec_temp);

  /* read vcom value */
  status = usec_get_vcom (ctx, &usec_vcom);
  if (status != USEC_DEV_OK)
    {
      printf ("[error] cannot read VCOM value\n\r");

      usec_deinit (ctx);
      return EXIT_FAILURE;
    }
  printf ("[log] screen VCOM: -%.2f [V]\n\r", (float) usec_vcom / 1000);

  /* cleanup screen - fullscreen update with UPDATE_MODE_INIT mode */
  usec_img_update (ctx, 0, 0, 1600, 1200, 0, 0);

  /* cleanup */
  usec_deinit (ctx);
  ctx = NULL;

  return EXIT_SUCCESS;
}
