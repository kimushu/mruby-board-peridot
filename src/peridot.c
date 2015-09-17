#include "mruby.h"
#include "mruby/variable.h"

extern struct RClass *hardware_init(mrb_state *mrb);
extern void hardware_final(mrb_state *mrb);

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

static mrb_value
peridot_name(mrb_state *mrb, mrb_value self)
{
  return mrb_str_new_cstr(mrb, "PERIDOT");
}

static mrb_value
peridot_author(mrb_state *mrb, mrb_value self)
{
  return mrb_str_new_cstr(mrb, "@s_osafune");
}

static mrb_value
peridot_website(mrb_state *mrb, mrb_value self)
{
  return mrb_str_new_cstr(mrb, "http://osafune.github.io/peridot.html");
}

void
mrb_mruby_board_peridot_gem_init(mrb_state *mrb)
{
  struct RClass *cls;
  struct RClass *hw_mod;

  hw_mod = hardware_init(mrb);

  //----------------------------------------------------------------
  // Peridot
  //
  cls = mrb_define_class(mrb, "Peridot", mrb->object_class);

  // Public methods for properties
  mrb_define_class_method(mrb, cls, "name"    , peridot_name    , MRB_ARGS_NONE());
  mrb_define_class_method(mrb, cls, "author"  , peridot_author  , MRB_ARGS_NONE());
  mrb_define_class_method(mrb, cls, "website" , peridot_website , MRB_ARGS_NONE());

  // Sub classes
  pfc_init(mrb, hw_mod, cls);

  // Board specific initialization
  peridot_class_init(mrb, cls);
}

void
mrb_mruby_board_peridot_gem_final(mrb_state *mrb)
{
  peridot_class_final(mrb);
  pfc_final(mrb);
  hardware_final(mrb);
}

/* vim: set et sts=2 sw=2: */
