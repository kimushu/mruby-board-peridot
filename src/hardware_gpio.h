#ifndef __HARDWARE_GPIO_H__
#define __HARDWARE_GPIO_H__

#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"

struct gpio_data;
typedef const char *(*gpio_rdfunc)(struct gpio_data *, uint32_t *);
typedef const char *(*gpio_wrfunc)(struct gpio_data *, uint32_t);

struct gpio_data {
  struct gpio_data *owner;  /* Copied */
  uint16_t refs;            /* Owner only / Zero for children */
  uint8_t msb;              /* Not shared */
  uint8_t lsb;              /* Not shared */
  uint32_t mask;            /* Not shared */
  uint32_t polarity;        /* Owner only */
  gpio_rdfunc get_value;    /* Copied */
  gpio_wrfunc set_value;    /* Copied */
  gpio_wrfunc clr_value;    /* Copied */
  gpio_wrfunc tgl_value;    /* Copied */
  gpio_rdfunc get_outen;    /* Copied */
  gpio_wrfunc set_outen;    /* Copied */
  gpio_wrfunc clr_outen;    /* Copied */

  /* Board specific members follow ... */
};

const struct mrb_data_type hardware_gpio_type;

#endif  /* !__HARDWARE_GPIO_H__ */
/* vim: set et sts=2 sw=2: */
