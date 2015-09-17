#include "hardware_gpio.h"
#include "mruby/variable.h"

struct pfc_regs {
  uint32_t direct_in;
  uint32_t direct_out;
  uint32_t out_func;
  uint32_t in_func;
};

struct pfc_gpio_data {
  struct gpio_data common;
  uint32_t gp_bits;         /* Owner only */
  struct pfc_regs *regs[4]; /* Copied */
};

#define PFC_PTR(p)    ((struct pfc_gpio_data *)(p))

#define DIRECT_IN(regs, mask) \
  (((mask) & 0xffu) ? (__builtin_ldwio(&(regs)->direct_in) & (mask) & 0xffu) : 0u)

#define DIRECT_OUT(regs, mask, value) \
  do { \
    if ((mask) & 0xffu) { \
      __builtin_stwio(&(regs)->direct_out, (((mask) & 0xffu) << 8) | ((value) & 0xffu)); \
    } \
  } while(0)

// 0bABCDEFGH -> 0b000A000B000C000D000E000F000G000H
#define BIT8_TO_32(v) \
  (((((v)&0x55u)*((1u<<18)|(1u<<12)|(1u<<6)|(1u<<0)))&0x01010101u)|\
   ((((v)&0xaau)*((1u<<21)|(1u<<15)|(1u<<9)|(1u<<3)))&0x10101010u))

// 0b000A000B000C000D000E000F000G000H -> 0bABCDEFGH
#define BIT32_TO_8(v) \
  (((((v)&0x01010101u)*((1u<<6)|(1u<<12)|(1u<<18)|(1u<<24)))|\
    (((v)&0x10101010u)*((1u<<3)|(1u<< 9)|(1u<<15)|(1u<<21))))>>24)

#define READ_OUTEN(regs, mask) \
  (((mask) & 0xffu) ? ({ \
    uint32_t t = __builtin_ldwio(&(regs)->out_func); \
    t |= ((t & 0xccccccccu) >> 2); \
    t |= (t >> 1); \
    BIT32_TO_8(t) & (mask); \
  }) : 0)

#define WRITE_OUTEN(regs, mask, en) \
  do { \
    if ((mask) & 0xffu) { \
      uint32_t m = BIT8_TO_32(mask); \
      uint32_t t = __builtin_ldwio(&(regs)->out_func); \
      __builtin_stwio(&(regs)->out_func, (t & ~(m * 0xfu)) | ((en) ? m : 0)); \
    } \
  } while(0)

static const char *
gpio_get_value(struct pfc_gpio_data *data, uint32_t *value)
{
  const uint32_t m = data->common.mask;
  *value =
    (DIRECT_IN(data->regs[0], m >>  0) <<  0) |
    (DIRECT_IN(data->regs[1], m >>  8) <<  8) |
    (DIRECT_IN(data->regs[2], m >> 16) << 16) |
    (DIRECT_IN(data->regs[3], m >> 22) << 22);
  return NULL;
}

static const char *
gpio_set_value(struct pfc_gpio_data *data, uint32_t bits)
{
  bits &= data->common.mask;
  DIRECT_OUT(data->regs[0], bits >>  0, 0xffu);
  DIRECT_OUT(data->regs[1], bits >>  8, 0xffu);
  DIRECT_OUT(data->regs[2], bits >> 16, 0xffu);
  DIRECT_OUT(data->regs[3], bits >> 22, 0xffu);
  return NULL;
}

static const char *
gpio_clr_value(struct pfc_gpio_data *data, uint32_t bits)
{
  bits &= data->common.mask;
  DIRECT_OUT(data->regs[0], bits >>  0, 0x00u);
  DIRECT_OUT(data->regs[1], bits >>  8, 0x00u);
  DIRECT_OUT(data->regs[2], bits >> 16, 0x00u);
  DIRECT_OUT(data->regs[3], bits >> 22, 0x00u);
  return NULL;
}

static const char *
gpio_tgl_value(struct pfc_gpio_data *data, uint32_t bits)
{
  uint32_t v;
  gpio_get_value(data, &v);
  v ^= ~0u;
  bits &= data->common.mask;
  DIRECT_OUT(data->regs[0], bits >>  0, v >>  0);
  DIRECT_OUT(data->regs[1], bits >>  8, v >>  8);
  DIRECT_OUT(data->regs[2], bits >> 16, v >> 16);
  DIRECT_OUT(data->regs[3], bits >> 22, v >> 22);
  return NULL;
}

static const char *
gpio_get_outen(struct pfc_gpio_data *data, uint32_t *value)
{
  const uint32_t m = data->common.mask;
  uint32_t v;
  v  = READ_OUTEN(data->regs[0], (m >>  0)) <<  0;
  v |= READ_OUTEN(data->regs[1], (m >>  8)) <<  8;
  v |= READ_OUTEN(data->regs[2], (m >> 16)) << 16;
  v |= READ_OUTEN(data->regs[3], (m >> 22)) << 22;
  *value = v;
  return NULL;
}

static const char *
gpio_set_outen(struct pfc_gpio_data *data, uint32_t bits)
{
  bits &= (data->common.mask & data->gp_bits);
  WRITE_OUTEN(data->regs[0], (bits >>  0) & 0xffu, 1);
  WRITE_OUTEN(data->regs[1], (bits >>  8) & 0xffu, 1);
  WRITE_OUTEN(data->regs[2], (bits >> 16) & 0x3fu, 1);
  WRITE_OUTEN(data->regs[3], (bits >> 22) & 0x3fu, 1);
  return NULL;
}

static const char *
gpio_clr_outen(struct pfc_gpio_data *data, uint32_t bits)
{
  bits &= (data->common.mask & data->gp_bits);
  WRITE_OUTEN(data->regs[0], (bits >>  0) & 0xffu, 0);
  WRITE_OUTEN(data->regs[1], (bits >>  8) & 0xffu, 0);
  WRITE_OUTEN(data->regs[2], (bits >> 16) & 0x3fu, 0);
  WRITE_OUTEN(data->regs[3], (bits >> 22) & 0x3fu, 0);
  return NULL;
}

static mrb_value
gpio_initialize(mrb_state *mrb, mrb_value self)
{
  const mrb_int width = 28;
  mrb_int base[4];
  mrb_int index;
  struct pfc_gpio_data *data;

  data = PFC_PTR(DATA_PTR(self));
  // if (data) {
  //   mrb_free(mrb, data->aux);
  //   mrb_free(mrb, data);
  // }
  mrb_data_init(self, NULL, &hardware_gpio_type);

  mrb_get_args(mrb, "iiii", &base[0], &base[1], &base[2], &base[3]);

  data = (struct pfc_gpio_data *)mrb_calloc(mrb, 1, sizeof(struct pfc_gpio_data));
  data->common.owner = &data->common;
  data->common.refs = 1;
  data->common.msb = width - 1;
  data->common.lsb = 0;
  data->common.mask = (1u << width) - 1;
  data->common.polarity = 0;
  data->common.get_value = (gpio_rdfunc)gpio_get_value;
  data->common.set_value = (gpio_wrfunc)gpio_set_value;
  data->common.clr_value = (gpio_wrfunc)gpio_clr_value;
  data->common.tgl_value = (gpio_wrfunc)gpio_tgl_value;
  data->common.get_outen = (gpio_rdfunc)gpio_get_outen;
  data->common.set_outen = (gpio_wrfunc)gpio_set_outen;
  data->common.clr_outen = (gpio_wrfunc)gpio_clr_outen;
  data->gp_bits = data->common.mask;
  for (index = 0; index < 4; ++index) {
    if (base[index] & (sizeof(struct pfc_regs) - 1)) {
      mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid base 0x%x", base[index]);
    }
    data->regs[index] = (struct pfc_regs *)base[index];
  }

  mrb_data_init(self, data, &hardware_gpio_type);
  return self;
}

static mrb_value
gpio_is_general_purpose(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (data->msb > data->lsb) {
    mrb_raisef(mrb, E_TYPE_ERROR, "cannot use is_general_purpose for bus signal");
  }
  if ((PFC_PTR(data->owner)->gp_bits & data->mask) == data->mask) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpio_is_special_function(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (data->msb > data->lsb) {
    mrb_raisef(mrb, E_TYPE_ERROR, "cannot use is_special_function for bus signal");
  }
  if ((PFC_PTR(data->owner)->gp_bits & data->mask) == 0) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static void
gpio_init(mrb_state *mrb, struct RClass *hw_mod, struct RClass *mod)
{
  struct RClass *base;
  struct RClass *cls;

  //----------------------------------------------------------------
  // PinFuncCtl.GPI
  //
  base = mrb_class_ptr(mrb_mod_cv_get(mrb, hw_mod, mrb_intern_cstr(mrb, "GPI")));
  cls = mrb_define_class_under(mrb, mod, "GPI", base);

  // Public methods for properties
  mrb_define_method(mrb, cls, "is_general_purpose"  , gpio_is_general_purpose , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_special_function" , gpio_is_special_function, MRB_ARGS_NONE());

  // Public methods
  mrb_define_method(mrb, cls, "initialize"          , gpio_initialize         , MRB_ARGS_REQ(4));

  // Aliases
  mrb_define_alias (mrb, cls, "general_purpose?"    , "is_general_purpose");
  mrb_define_alias (mrb, cls, "special_function?"   , "is_special_function");

  //----------------------------------------------------------------
  // PinFuncCtl.GPIO
  //
  base = mrb_class_ptr(mrb_mod_cv_get(mrb, hw_mod, mrb_intern_cstr(mrb, "GPIO")));
  cls = mrb_define_class_under(mrb, mod, "GPIO", base);

  // Public methods for properties
  mrb_define_method(mrb, cls, "is_general_purpose"  , gpio_is_general_purpose , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_special_function" , gpio_is_special_function, MRB_ARGS_NONE());

  // Public methods
  mrb_define_method(mrb, cls, "initialize"          , gpio_initialize         , MRB_ARGS_REQ(4));

  // Aliases
  mrb_define_alias (mrb, cls, "general_purpose?"    , "is_general_purpose");
  mrb_define_alias (mrb, cls, "special_function?"   , "is_special_function");
}

static void
gpio_final(mrb_state *mrb)
{
  /* do nothing */
}

void
pfc_init(mrb_state *mrb, struct RClass *hw_mod, struct RClass *mod)
{
  struct RClass* cls;

  //----------------------------------------------------------------
  // Peridot.PinFuncCtl
  //
  cls = mrb_define_class_under(mrb, mod, "PinFuncCtl", mrb->object_class);

  gpio_init(mrb, hw_mod, cls);
}

void
pfc_final(mrb_state *mrb)
{
  gpio_final(mrb);
}

/* vim: set et sts=2 sw=2: */
