#include "mruby.h"
#include "mruby/variable.h"

__attribute__((weak)) void
peridot_class_init(mrb_state *mrb, struct RClass *cls)
{
  /* This is a placeholder for override by board specific code */
}

__attribute__((weak)) void
peridot_class_final(mrb_state *mrb)
{
  /* This is a placeholder for override by board specific code */
}

void
mrb_mruby_board_peridot_gem_init(mrb_state *mrb)
{
  struct RClass *cls;

  cls = mrb_define_class(mrb, "Peridot", mrb->object_class);

  mrb_define_const(mrb, cls, "Name", mrb_str_new_cstr(mrb, "PERIDOT"));
  mrb_define_const(mrb, cls, "Author", mrb_str_new_cstr(mrb, "@s_osafune"));
  mrb_define_const(mrb, cls, "Website", mrb_str_new_cstr(mrb, "https://peridotcraft.com/"));

  peridot_class_init(mrb, cls);
}

void
mrb_mruby_board_peridot_gem_final(mrb_state *mrb)
{
  peridot_class_final(mrb);
}
