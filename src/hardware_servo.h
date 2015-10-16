#ifndef __HARDWARE_SERVO_H__
#define __HARDWARE_SERVO_H__

#include "mruby.h"
#include "mruby/class.h"
#include "mruby/data.h"

struct servo_data;
typedef const char *(*servo_rdfunc)(struct servo_data *, int *);
typedef const char *(*servo_wrfunc)(struct servo_data *, int);

struct servo_data {
  int channel;
  uint8_t calib_min;
  uint8_t calib_max;
  servo_wrfunc cfg_pin;
  servo_rdfunc get_enabled;
  servo_wrfunc let_enabled;
  servo_rdfunc get_position;
  servo_wrfunc let_position;

  /* Board specific members follow ... */
};

const struct mrb_data_type hardware_servo_type;

#endif  /* !__HARDWARE_SERVO_H__ */
/* vim: set et sts=2 sw=2: */
