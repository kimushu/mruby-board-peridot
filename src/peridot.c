#include "mruby.h"
#include "mruby/variable.h"

extern const void *__peridot_start_led_base;
extern const void *__peridot_digital_io_base;
extern const int __peridot_digital_io_width;

static mrb_value
piocore_new(mrb_state *mrb, mrb_int base, mrb_int width)
{
  static mrb_value cls;

  if (!mrb_test(cls)) {
    mrb_value mod;
    mod = mrb_const_get(mrb, mrb_obj_value(mrb->object_class), mrb_intern_lit(mrb, "Altera"));
    cls = mrb_const_get(mrb, mod, mrb_intern_lit(mrb, "PIOCore"));
  }

  return mrb_funcall(mrb, cls, "new", 2, mrb_fixnum_value(base), mrb_fixnum_value(width));
}

static mrb_value
peridot_start_led(mrb_state *mrb, mrb_value self)
{
  static mrb_value start_led;

  if (!mrb_test(start_led)) {
    start_led = piocore_new(mrb, (mrb_int)__peridot_start_led_base, 1);
  }
  return start_led;
}

static mrb_value
peridot_digital_io(mrb_state *mrb, mrb_value self)
{
  static mrb_value digital_io;

  if (!mrb_test(digital_io)) {
    digital_io = piocore_new(mrb,
                    (mrb_int)__peridot_digital_io_base,
                    (mrb_int)__peridot_digital_io_width);
  }
  return digital_io;
}

void
mrb_board_peridot_gem_init(mrb_state *mrb)
{
  struct RClass *cls;

  cls = mrb_define_class(mrb, "Peridot", mrb->object_class);

  mrb_define_const(mrb, cls, "Name", mrb_str_new_cstr(mrb, "PERIDOT"));
  mrb_define_const(mrb, cls, "Author", mrb_str_new_cstr(mrb, "@s_osafune"));
  mrb_define_const(mrb, cls, "Website", mrb_str_new_cstr(mrb, "https://peridotcraft.com/"));

  mrb_define_class_method(mrb, cls, "start_led" , peridot_start_led , MRB_ARGS_NONE());
  mrb_define_class_method(mrb, cls, "digital_io", peridot_digital_io, MRB_ARGS_NONE());
}

void
mrb_board_peridot_gem_final(mrb_state *mrb)
{
}
