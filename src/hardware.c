#include "mruby.h"
#include "mruby/class.h"

extern void hardware_gpio_init(mrb_state *mrb, struct RClass *mod);
extern void hardware_gpio_final(mrb_state *mrb);

struct RClass *
hardware_init(mrb_state *mrb)
{
  struct RClass *mod;

  mod = mrb_define_module(mrb, "Hardware");
  hardware_gpio_init(mrb, mod);

  return mod;
}

void
hardware_final(mrb_state *mrb)
{
  hardware_gpio_final(mrb);
}

/* vim: set et sts=2 sw=2: */
