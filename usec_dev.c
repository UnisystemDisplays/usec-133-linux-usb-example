#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <fcntl.h>
#include <scsi/sg.h>
#include <sys/ioctl.h>
#include "usec_dev.h"

/******************************************************************************/

#define IT8951_USB_INQUIRY            (0x12)
#define IT8951_USB_OP_GET_SYS         (0x80)
#define IT8951_USB_OP_READ_MEM        (0x81)
#define IT8951_USB_OP_WRITE_MEM       (0x82)
#define IT8951_USB_OP_READ_REG        (0x83)
#define IT8951_USB_OP_WRITE_REG       (0x84)
#define IT8951_USB_OP_DPY_AREA        (0x94)
#define IT8951_USB_OP_LD_IMG_AREA     (0xA2)
#define IT8951_USB_OP_PMIC_CTL        (0xA3)
#define IT8951_USB_OP_FSET_TEMP       (0xA4)
#define IT8951_USB_OP_FAST_WRITE_MEM  (0xA5)
#define IT8951_USB_OP_AUTO_RESET      (0xA7)

/******************************************************************************/

typedef struct
{
  uint32_t standard_cmd_no;
  uint32_t extend_cmd_no;
  uint32_t signature;
  uint32_t version;
  uint32_t width;
  uint32_t height;
  uint32_t update_buf_base;
  uint32_t image_buf_base;
  uint32_t temperature_no;
  uint32_t mode_no;
  uint32_t frame_count[8];
  uint32_t num_img_buf;
  uint32_t wbf_addr;
  uint32_t reserved[9];
  void *cmd_info_data[1];
} it8951_sys_info;

typedef struct
{
  uint32_t mem_addr;
  uint32_t wav_mode;
  uint32_t pos_x;
  uint32_t pos_y;
  uint32_t width;
  uint32_t height;
  uint32_t engine_index;
} it8951_disp_arg;

typedef struct
{
  uint32_t addr;
  uint32_t x;
  uint32_t y;
  uint32_t w;
  uint32_t h;
} it8951_load_arg;

typedef struct
{
  uint8_t set;
  uint8_t val;
} it8951_temp_arg;

typedef struct sg_io_hdr it8951_sg_io_hdr;

enum {
  IT8951_TEMP_GET,
  IT8951_TEMP_SET
};

/******************************************************************************/

/*
 * init_io_hdr()
 */
static it8951_sg_io_hdr *
init_io_hdr (void)
{
  it8951_sg_io_hdr *hdr;

  hdr = (it8951_sg_io_hdr*) malloc (sizeof(it8951_sg_io_hdr));
  memset (hdr, 0, sizeof(it8951_sg_io_hdr));
  if (hdr)
    {
      hdr->interface_id = 'S';
      hdr->flags = SG_FLAG_LUN_INHIBIT;
    }

  return hdr;
}

/*
 * destroy_io_hdr()
 */
static void
destroy_io_hdr (it8951_sg_io_hdr  *hdr)
{
  if (hdr)
    free(hdr);
}

/*
 * set_xfer_data()
 */
static void
set_xfer_data (it8951_sg_io_hdr  *hdr,
               void              *data,
               uint32_t           length)
{
  if (hdr)
    {
      hdr->dxferp = data;
      hdr->dxfer_len = length;
    }
}

/*
 * set_sense_data()
 */
static void
set_sense_data (it8951_sg_io_hdr  *hdr,
                uint8_t           *data,
                uint32_t           length)
{
  if (hdr)
    {
      hdr->sbp = data;
      hdr->mx_sb_len = length;
    }
}

/*
 * data_swap_32()
 */
static uint32_t
data_swap_32 (uint32_t input)
{
  return ((input & 0xFF) << 24) | ((input >> 8 & 0xFF) << 16) | \
          ((input >> 16 & 0xFF) << 8) | (input >> 24 & 0xFF);
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/

/*
 * scsi_it8951_cmd_inquiry()
 */
static uint8_t
scsi_it8951_cmd_inquiry (int                fd,
                         it8951_sg_io_hdr  *hdr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = IT8951_USB_INQUIRY;
          break;

          default:
            cdb[i] = 0x00;
          break;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_FROM_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_system_info()
 */
static uint8_t
scsi_it8951_cmd_system_info (int                fd,
                             it8951_sg_io_hdr  *hdr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 2:
            cdb[i] = 0x38;
          break;

          case 3:
            cdb[i] = 0x39;
          break;

          case 4:
            cdb[i] = 0x35;
          break;

          case 5:
            cdb[i] = 0x31;
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_GET_SYS;
          break;

          case 8:
            cdb[i] = 0x01;
          break;

          case 10:
            cdb[i] = 0x02;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_FROM_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_read_mem()
 */
static uint8_t
scsi_it8951_cmd_read_mem (int                fd,
                          it8951_sg_io_hdr  *hdr,
                          uint32_t           addr,
                          uint16_t           length)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 2:
            cdb[i] = (uint8_t)((addr >> 24) & 0xFF);
          break;

          case 3:
            cdb[i] = (uint8_t)((addr >> 16) & 0xFF);
          break;

          case 4:
            cdb[i] = (uint8_t)((addr >> 8) & 0xFF);
          break;

          case 5:
            cdb[i] = (uint8_t)(addr & 0xFF);
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_READ_MEM;
          break;

          case 7:
            cdb[i] = (uint8_t)((length >> 8) & 0xFF);
          break;

          case 8:
            cdb[i] = (uint8_t)(length & 0xFF);
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_FROM_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_write_mem()
 */
static uint8_t
scsi_it8951_cmd_write_mem (int                fd,
                           it8951_sg_io_hdr  *hdr,
                           uint32_t           addr,
                           uint16_t           length)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 2:
            cdb[i] = (uint8_t)((addr >> 24) & 0xFF);
          break;

          case 3:
            cdb[i] = (uint8_t)((addr >> 16) & 0xFF);
          break;

          case 4:
            cdb[i] = (uint8_t)((addr >> 8) & 0xFF);
          break;

          case 5:
            cdb[i] = (uint8_t)(addr & 0xFF);
          break;

          case 6:
#if USEC_DEV_FAST_WRITE
            cdb[i] = IT8951_USB_OP_FAST_WRITE_MEM;
#else
            cdb[i] = IT8951_USB_OP_WRITE_MEM;
#endif
          break;

          case 7:
            cdb[i] = (uint8_t)((length >> 8) & 0xFF);
          break;

          case 8:
            cdb[i] = (uint8_t)(length & 0xFF);
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_TO_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_read_reg()
 */
static uint8_t
scsi_it8951_cmd_read_reg (int                fd,
                          it8951_sg_io_hdr  *hdr,
                          uint32_t           addr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 2:
            cdb[i] = (uint8_t)((addr >> 24) & 0xFF);
          break;

          case 3:
            cdb[i] = (uint8_t)((addr >> 16) & 0xFF);
          break;

          case 4:
            cdb[i] = (uint8_t)((addr >> 8) & 0xFF);
          break;

          case 5:
            cdb[i] = (uint8_t)(addr & 0xFF);
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_READ_REG;
          break;

          case 8:
            cdb[i] = 0x04;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_FROM_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_write_reg()
 */
static uint8_t
scsi_it8951_cmd_write_reg (int                fd,
                           it8951_sg_io_hdr  *hdr,
                           uint32_t           addr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 2:
            cdb[i] = (uint8_t)((addr >> 24) & 0xFF);
          break;

          case 3:
            cdb[i] = (uint8_t)((addr >> 16) & 0xFF);
          break;

          case 4:
            cdb[i] = (uint8_t)((addr >> 8) & 0xFF);
          break;

          case 5:
            cdb[i] = (uint8_t)(addr & 0xFF);
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_WRITE_REG;
          break;

          case 8:
            cdb[i] = 0x04;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_TO_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_dpy_area()
 */
static uint8_t
scsi_it8951_cmd_dpy_area (int                fd,
                          it8951_sg_io_hdr  *hdr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_DPY_AREA;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_TO_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_load_img()
 */
static uint8_t
scsi_it8951_cmd_load_img (int                fd,
                          it8951_sg_io_hdr  *hdr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_LD_IMG_AREA;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_TO_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_get_set_temp()
 */
static uint8_t
scsi_it8951_cmd_get_set_temp (int                fd,
                              it8951_sg_io_hdr  *hdr,
                              uint8_t            temp_option,
                              uint8_t            temp_value)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_FSET_TEMP;
          break;

          case 7:
            cdb[i] = temp_option;
          break;

          case 8:
            cdb[i] = temp_value;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_FROM_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_set_pmic()
 */
static uint8_t
scsi_it8951_cmd_set_pmic (int                fd,
                          it8951_sg_io_hdr  *hdr,
                          uint16_t           vcom,
                          uint8_t            set_vcom,
                          uint8_t            set_power,
                          uint8_t            on_off)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_PMIC_CTL;
          break;

          case 7:
            cdb[i] = (uint8_t)((vcom & 0xFF00) >> 8);
          break;

          case 8:
            cdb[i] = (uint8_t)(vcom & 0xFF);
          break;

          case 9:
            cdb[i] = set_vcom;
          break;

          case 10:
            cdb[i] = set_power;
          break;

          case 11:
            cdb[i] = on_off;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_FROM_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/*
 * scsi_it8951_cmd_auto_reset()
 */
static uint8_t
scsi_it8951_cmd_auto_reset (int                fd,
                            it8951_sg_io_hdr  *hdr)
{
  uint8_t cdb[16];

  hdr->cmd_len = 16;
  for (uint8_t i = 0; i < (hdr->cmd_len); i++)
    {
      switch(i)
        {
          case 0:
            cdb[i] = 0xFE;
          break;

          case 6:
            cdb[i] = IT8951_USB_OP_AUTO_RESET;
          break;

          default:
            cdb[i] = 0x00;
        }
    }

  hdr->cmdp = cdb;
  hdr->dxfer_direction = SG_DXFER_TO_DEV;

  if (ioctl (fd, SG_IO, hdr) < 0)
    return USEC_DEV_ERR;

  return USEC_DEV_OK;
}

/******************************************************************************/

/*
 * it8951_cmd_inquiry()
 */
static uint8_t
it8951_cmd_inquiry (usec_ctx  *ctx,
                    uint8_t    id)
{
  it8951_sg_io_hdr *hdr;
  uint8_t data_buffer[USEC_DEV_BLOCK_LEN*256];
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, data_buffer, USEC_DEV_BLOCK_LEN*256);
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_inquiry (ctx->dev_fd[id], hdr);

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_system_info()
 */
static uint8_t
it8951_cmd_system_info (usec_ctx         *ctx,
                        uint8_t           id,
                        it8951_sys_info  *info)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, info, (sizeof(it8951_sys_info) / sizeof(uint32_t)));
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_system_info (ctx->dev_fd[id], hdr);
  if (status == USEC_DEV_OK)
    {
      it8951_sys_info *data = hdr->dxferp;

      info->standard_cmd_no = data_swap_32 (data->standard_cmd_no);
      info->extend_cmd_no   = data_swap_32 (data->extend_cmd_no);
      info->signature       = data_swap_32 (data->signature);
      info->version         = data_swap_32 (data->version);
      info->width           = data_swap_32 (data->width);
      info->height          = data_swap_32 (data->height);
      info->update_buf_base = data_swap_32 (data->update_buf_base);
      info->image_buf_base  = data_swap_32 (data->image_buf_base);
      info->temperature_no  = data_swap_32 (data->temperature_no);
      info->mode_no         = data_swap_32 (data->mode_no);

      for (uint8_t i = 0; i < 8; i++)
        info->frame_count[i] = data_swap_32 (data->frame_count[i]);

      info->num_img_buf = data_swap_32 (data->num_img_buf);
      info->wbf_addr    = data_swap_32 (data->wbf_addr);

      for (uint8_t i = 0; i < 9; i++)
        info->reserved[i] = data_swap_32 (data->reserved[i]);
    }

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_read_mem()
 */
static uint8_t
it8951_cmd_read_mem (usec_ctx  *ctx,
                     uint8_t    id,
                     uint32_t   addr,
                     uint16_t   length,
                     uint8_t   *buf)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, buf, length);
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_read_mem (ctx->dev_fd[id], hdr, addr, length);

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_write_mem()
 */
static uint8_t
it8951_cmd_write_mem (usec_ctx  *ctx,
                      uint8_t    id,
                      uint32_t   addr,
                      uint16_t   length,
                      uint8_t   *buf)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, buf, length);
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_write_mem (ctx->dev_fd[id], hdr, addr, length);

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_read_reg()
 */
static uint8_t
it8951_cmd_read_reg (usec_ctx  *ctx,
                     uint8_t    id,
                     uint32_t   addr,
                     uint32_t  *buf)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, buf, sizeof(uint32_t));
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_read_reg (ctx->dev_fd[id], hdr, addr);

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_write_reg()
 */
static uint8_t
it8951_cmd_write_reg (usec_ctx  *ctx,
                      uint8_t    id,
                      uint32_t   addr,
                      uint32_t   buf)
{
  it8951_sg_io_hdr *hdr;
  uint32_t buf_in;
  uint8_t status;

  hdr = init_io_hdr();
  buf_in = data_swap_32 (buf);
  set_xfer_data (hdr, &buf_in, sizeof(uint32_t));
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_write_reg (ctx->dev_fd[id], hdr, addr);

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_dpy_area()
 */
static uint8_t
it8951_cmd_dpy_area (usec_ctx  *ctx,
                     uint8_t    id,
                     uint32_t   pos_x,
                     uint32_t   pos_y,
                     uint32_t   width,
                     uint32_t   height,
                     uint32_t   wav_mode,
                     uint32_t   wait_ready)
{
  it8951_sg_io_hdr *hdr;
  it8951_disp_arg displayArg;
  uint8_t status;

  displayArg.pos_x        = data_swap_32 (pos_x);
  displayArg.pos_y        = data_swap_32 (pos_y);
  displayArg.width        = data_swap_32 (width);
  displayArg.height       = data_swap_32 (height);
  displayArg.engine_index = data_swap_32 (wait_ready);
  displayArg.mem_addr     = data_swap_32 (ctx->dev_addr[id]);
  displayArg.wav_mode     = data_swap_32 (wav_mode);

  hdr = init_io_hdr();
  set_xfer_data (hdr, &displayArg, sizeof(it8951_disp_arg));
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_dpy_area (ctx->dev_fd[id], hdr);

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_load_img()
 */
static uint8_t
it8951_cmd_load_img (usec_ctx  *ctx,
                     uint8_t    id,
                     uint8_t   *src_img,
                     uint32_t   pos_x,
                     uint32_t   pos_y,
                     uint32_t   width,
                     uint32_t   height)
{
  uint8_t status;
  uint32_t counter;

  counter = (USEC_DEV_SPT_LEN / width);
  status = 0;

  if (width <= 2048 && width != (ctx->dev_width[id]))
    {
      it8951_sg_io_hdr  *hdr;
      it8951_load_arg    load_arg;

      hdr = init_io_hdr();
      for (uint32_t i = 0; i < height; i += counter)
        {
          uint8_t *buf;

          if (counter > (height-i))
            counter = (height-i);

          buf = malloc((sizeof(it8951_load_arg)+(width*counter)));

          load_arg.x    = data_swap_32 (pos_x);
          load_arg.y    = data_swap_32 (pos_y + i);
          load_arg.w    = data_swap_32 (width);
          load_arg.h    = data_swap_32 (counter);
          load_arg.addr = data_swap_32 (ctx->dev_addr[id]);

          memcpy (buf, &load_arg, sizeof(it8951_load_arg));
          memcpy ((buf + sizeof(it8951_load_arg)),
                  src_img+(i*width), width*counter);

          set_xfer_data (hdr, buf, sizeof(it8951_load_arg) + (width*counter));
          set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

          status |= scsi_it8951_cmd_load_img (ctx->dev_fd[id], hdr);
          free(buf);
        }

      destroy_io_hdr(hdr);
    }
  else
    {
      for (uint32_t i = 0; i < height; i += counter)
        {
          if (counter > (height-i))
            counter = (height-i);

          status |= it8951_cmd_write_mem (ctx, id,
                    (ctx->dev_addr[id] + pos_x + ((pos_y + i) * \
                    (ctx->dev_width[id]))),
                    (uint32_t)(width * counter), (src_img + (i * width)));
        }
    }

  return status;
}

/*
 * it8951_cmd_get_set_temp()
 */
static uint8_t
it8951_cmd_get_set_temp (usec_ctx         *ctx,
                         uint8_t           id,
                         it8951_temp_arg  *temp)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status, flag;

  flag = temp->set;
  hdr = init_io_hdr();

  set_xfer_data (hdr, temp, sizeof(it8951_temp_arg) / sizeof(uint8_t));
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_get_set_temp (ctx->dev_fd[id],
                                         hdr, temp->set,temp->val);
  if (status == USEC_DEV_OK)
    {
      if (flag == IT8951_TEMP_GET)
        {
          it8951_temp_arg *temp_tmp;
          uint8_t temp_byte;

          /* swap */
          temp_tmp = hdr->dxferp;
          temp_byte = temp_tmp->set;
          temp_tmp->set = temp_tmp->val;
          temp_tmp->val = temp_byte;
        }
    }

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_get_set_pmic()
 */
static uint8_t
it8951_cmd_get_set_pmic (usec_ctx  *ctx,
                         uint8_t    id,
                         uint16_t   vcom_set_value,
                         uint16_t  *vcom_get_value,
                         uint8_t    do_set_vcom,
                         uint8_t    do_set_power,
                         uint8_t    power_on_off)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, vcom_get_value, sizeof (uint16_t));
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_set_pmic(ctx->dev_fd[id], hdr, vcom_set_value,
                                    do_set_vcom, do_set_power, power_on_off);
if (status == USEC_DEV_OK)
    {
      if (vcom_set_value == 0xffff)
        {
          /* swap */
          *vcom_get_value = (*vcom_get_value >> 8) | (*vcom_get_value << 8);
        }
    }

  destroy_io_hdr(hdr);
  return status;
}

/*
 * it8951_cmd_auto_reset()
 */
static uint8_t
it8951_cmd_auto_reset (usec_ctx  *ctx,
                       uint8_t    id)
{
  it8951_sg_io_hdr *hdr;
  uint8_t status;

  hdr = init_io_hdr();
  set_xfer_data (hdr, NULL, 0);
  set_sense_data (hdr, ctx->dev_sense_buf, USEC_DEV_SENSE_LEN);

  status = scsi_it8951_cmd_auto_reset (ctx->dev_fd[id], hdr);

  destroy_io_hdr(hdr);
  return status;
}

/******************************************************************************/

/*
 * usec_dev_log()
 */
static void
usec_dev_log (const char* fmt, ...)
{
#if USEC_DEV_DEBUG_LOG
  va_list args;

  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
#endif
}

/******************************************************************************/

/*
 * usec_init()
 */
usec_ctx *
usec_init (const uint8_t *dev_path)
{
  usec_ctx *ctx;
  it8951_sys_info info;
  uint8_t *path;
  uint8_t status;

  /* init usec context */
  ctx = malloc(sizeof(*ctx));
  if (ctx == NULL)
    {
      usec_dev_log ("[usec] error: cannot initialize device context\n\r");
      return NULL;
    }

  /* init sense buffer */
  ctx->dev_sense_buf = malloc(USEC_DEV_SENSE_LEN);
  if (ctx->dev_sense_buf == NULL)
    {
      usec_dev_log ("[usec] error: cannot initialize device context\n\r");

      free (ctx);
      return NULL;
    }
  memset (ctx->dev_sense_buf, 0, USEC_DEV_SENSE_LEN);

  /* set device path */
  path = dev_path ? (uint8_t*)dev_path : (uint8_t*)"/dev/eink_usec_133BWN0";

  /* open usec device */
  ctx->dev_fd[0] = open ((char*)path, O_RDWR);
  if (ctx->dev_fd[0] <= 0)
    {
      usec_dev_log ("[usec] error: cannot open '%s' device\n\r", dev_path);

      free (ctx->dev_sense_buf);
      free (ctx);
      return NULL;
    }

  /* send 'inquiry' command */
  status = it8951_cmd_inquiry (ctx, 0);
  if (status != 0)
    {
      usec_dev_log ("[usec] error: cannot send 'inquiry' command\n\r");

      close (ctx->dev_fd[0]);
      free (ctx->dev_sense_buf);
      free (ctx);
      return NULL;
    }

  /* get system info */
  status = it8951_cmd_system_info (ctx, 0, &info);
  if (status != 0)
    {
      usec_dev_log ("[usec] error: cannot read data from controller\n\r");

      close (ctx->dev_fd[0]);
      free (ctx->dev_sense_buf);
      free (ctx);
      return NULL;
    }

  ctx->dev_width[0]  = info.width;
  ctx->dev_height[0] = info.height;
  ctx->dev_addr[0]   = info.image_buf_base;

  usec_dev_log ("[usec] status: screen width - %d [px]\n\r",
                ctx->dev_width[0]);
  usec_dev_log ("[usec] status: screen height - %d [px]\n\r",
                ctx->dev_height[0]);

  return ctx;
}

/*
 * usec_deinit()
 */
void
usec_deinit (usec_ctx *ctx)
{
  if (ctx == NULL)
    {
      usec_dev_log ("[usec] error: invalid device context\n\r");
      return;
    }

  close (ctx->dev_fd[0]);
  free (ctx->dev_sense_buf);
  free (ctx);
}

/*
 * usec_get_temp()
 */
uint8_t
usec_get_temp (usec_ctx  *ctx,
               uint8_t   *temp_val)
{
  it8951_temp_arg temp;
  uint8_t status;

  if (ctx == NULL)
    {
      usec_dev_log ("[usec] error: invalid device context\n\r");
      return USEC_DEV_ERR;
    }

  temp.set = IT8951_TEMP_GET;
  status = it8951_cmd_get_set_temp (ctx, 0, &temp);
  if (status == USEC_DEV_OK)
    {
      usec_dev_log ("[usec] status: screen temp - %d [degC]\n\r", temp.val);
      if (temp_val != NULL)
        *temp_val = temp.val;
    }
  else
    {
      usec_dev_log ("[usec] error: cannot read data from temperature " \
                    "sensor\n\r");
    }

  return status;
}

/*
 * usec_get_vcom()
 */
uint8_t
usec_get_vcom (usec_ctx  *ctx,
               uint16_t  *vcom_val)
{
  uint16_t vcom;
  uint8_t status;

  if (ctx == NULL)
    {
      usec_dev_log ("[usec] error: invalid device context\n\r");
      return USEC_DEV_ERR;
    }

  status = it8951_cmd_get_set_pmic (ctx, 0, 0xFFFF, &vcom, 0, 0, 0);
  if (status == USEC_DEV_OK)
    {
      usec_dev_log ("[usec] status: screen vcom - -%.2f [V]\n\r",
                    (float)vcom/1000);
      if (vcom_val != NULL)
        *vcom_val = vcom;
    }
  else
    {
      usec_dev_log ("[usec] error: cannot read vcom value\n\r");
    }

  return status;
}

/*
 * usec_img_upload()
 */
uint8_t
usec_img_upload (usec_ctx  *ctx,
                 uint8_t   *img_data,
                 size_t     img_size,
                 uint8_t    img_bpp,
                 uint32_t   img_pos_x,
                 uint32_t   img_pos_y,
                 uint32_t   img_width,
                 uint32_t   img_height)
{
  uint8_t status;

  if (ctx == NULL)
    {
      usec_dev_log ("[usec] error: invalid device context\n\r");
      return USEC_DEV_ERR;
    }

  if (img_bpp != IMG_8BPP)
    {
      usec_dev_log ("[usec] error: usb lib supports only IMG_8BPP mode\n\r");
      return USEC_DEV_ERR;
    }

  if ((img_width == 0) || (img_height == 0))
    {
      usec_dev_log ("[usec] error: invalid image data width/height\n\r");
      return USEC_DEV_ERR;
    }

  if ((img_size > (8 * ctx->dev_width[0] * ctx->dev_width[0])) || \
      (img_width * img_height) != img_size)
    {
      usec_dev_log ("[usec] error: invalid image data size\n\r");
      return USEC_DEV_ERR;
    }

  if ((img_pos_x + img_width > ctx->dev_width[0]) || \
      (img_pos_y + img_height > ctx->dev_height[0]))
    {
      usec_dev_log ("[usec] error: image out of screen bounds\n\r");
      return USEC_DEV_ERR;
    }

  status = it8951_cmd_load_img (ctx, 0, img_data,
                                img_pos_x, img_pos_y,
                                img_width, img_height);
  if (status == USEC_DEV_OK)
    {
      usec_dev_log ("[usec] status: uploading image - " \
                    "%04d [pos_x] %04d [pos_y] %04d [width] %04d [height]\n\r",
                    img_pos_x, img_pos_y, img_width, img_height);
    }
  else
    {
       usec_dev_log ("[usec] error: cannot upload image data\n\r");
    }

  return status;
}

/*
 * usec_img_update()
 */
uint8_t
usec_img_update (usec_ctx  *ctx,
                 uint32_t   area_pos_x,
                 uint32_t   area_pos_y,
                 uint32_t   area_width,
                 uint32_t   area_height,
                 uint8_t    update_mode,
                 uint8_t    update_wait)
{
  uint8_t status;

  if (ctx == NULL)
    {
      usec_dev_log ("[usec] error: invalid device context\n\r");
      return USEC_DEV_ERR;
    }

  if (update_mode > 7)
    {
      usec_dev_log ("[usec] error: invalid update mode value\n\r");
      return USEC_DEV_ERR;
    }

  if ((area_width == 0) || (area_height == 0))
    {
      usec_dev_log ("[usec] error: invalid update area width/height\n\r");
      return USEC_DEV_ERR;
    }

  if ((area_pos_x > ctx->dev_width[0]) || \
      (area_pos_y > ctx->dev_height[0]))
    {
      usec_dev_log ("[usec] error: update area out of screen bounds\n\r");
      return USEC_DEV_ERR;
    }

  if ((area_pos_x + area_width > ctx->dev_width[0]) || \
      (area_pos_y + area_height > ctx->dev_height[0]))
    {
      usec_dev_log ("[usec] error: update area out of screen bounds\n\r");
      return USEC_DEV_ERR;
    }

  status = it8951_cmd_dpy_area (ctx, 0, area_pos_x, area_pos_y,
                                area_width, area_height,
                                update_mode, update_wait);
  if (status == USEC_DEV_OK)
    {
      usec_dev_log ("[usec] status: updating area - " \
                    "%04d [pos_x] %04d [pos_y] %04d [width] %04d [height]\n\r",
                    area_pos_x, area_pos_y, area_width, area_height);
    }
  else
    {
       usec_dev_log ("[usec] error: cannot update selected display area\n\r");
    }

  return it8951_cmd_get_set_pmic (ctx, 0, 0, NULL, 0, 1, 0);
}

/******************************************************************************/
