#include "hardware_gpio.h"
#include "mruby/range.h"

static void
gpio_free(mrb_state *mrb, void *ptr)
{
  struct gpio_data *data;
  data = (struct gpio_data *)ptr;

  if (data->refs == 0) {
    /* Not an owner */
    gpio_free(mrb, data->owner);
    mrb_free(mrb, data);
  }
  else if (data->refs == 1) {
    /* Disappearing owner */
    mrb_free(mrb, data);
  }
  else {
    --data->owner->refs;
  }
}

const struct mrb_data_type hardware_gpio_type = {"Hardware.GPIO", gpio_free};

static mrb_value
gpi_is_active_high(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (data->msb > data->lsb) {
    mrb_raisef(mrb, E_TYPE_ERROR, "cannot use is_active_high for bus signal");
  }
  if ((data->owner->polarity & data->mask) == 0) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpi_is_active_low(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (data->msb > data->lsb) {
    mrb_raisef(mrb, E_TYPE_ERROR, "cannot use is_active_low for bus signal");
  }
  if ((data->owner->polarity & data->mask) == data->mask) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpi_is_asserted(mrb_state *mrb, mrb_value self)
{
  const char *error = "cannot use is_asserted for bus signal";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if ((data->msb > data->lsb) || (error = (*data->get_value)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  if (((value ^ data->owner->polarity) & data->mask) == data->mask) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpi_is_high(mrb_state *mrb, mrb_value self)
{
  const char *error = "cannot use is_high for bus signal";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if ((data->msb > data->lsb) || (error = (*data->get_value)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  if ((value & data->mask) == data->mask) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpi_is_low(mrb_state *mrb, mrb_value self)
{
  const char *error = "cannot use is_low for bus signal";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if ((data->msb > data->lsb) || (error = (*data->get_value)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  if ((value & data->mask) == 0) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpi_is_negated(mrb_state *mrb, mrb_value self)
{
  const char *error = "cannot use is_negated for bus signal";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if ((data->msb > data->lsb) || (error = (*data->get_value)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  if (((value ^ data->owner->polarity) & data->mask) == 0) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpi_is_output_disabled(mrb_state *mrb, mrb_value self)
{
  return mrb_true_value();
}

static mrb_value
gpi_is_output_enabled(mrb_state *mrb, mrb_value self)
{
  return mrb_false_value();
}

static mrb_value
gpi_value(mrb_state *mrb, mrb_value self)
{
#if defined(MRB_WORD_BOXING)
  const mrb_int max_width = (MRB_INT_BIT - MRB_FIXNUM_SHIFT);
#else
  const mrb_int max_width = (MRB_INT_BIT);
#endif
  const char *error = "width is too large to treat as mrb_int";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (((data->msb - data->lsb + 1) > max_width) ||
      (error = (*data->get_value)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return mrb_fixnum_value((mrb_int)((value & data->mask) / (1u << data->lsb)));
}

static mrb_value
gpi_width(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  return mrb_fixnum_value(data->msb - data->lsb + 1);
}

static mrb_value
gpi_active_high(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  data->owner->polarity &= ~data->mask;
  return self;
}

static mrb_value
gpi_active_low(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  data->owner->polarity |= data->mask;
  return self;
}

static mrb_value
gpi_slice(mrb_state *mrb, mrb_value self)
{
  struct gpio_data *src_data, *new_data;
  mrb_int msb, lsb;
  mrb_value arg_msb, arg_lsb;

  src_data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);

  arg_lsb = mrb_nil_value();
  mrb_get_args(mrb, "o|o", &arg_msb, &arg_lsb);

  msb = lsb = -1;
  if (mrb_type(arg_msb) == MRB_TT_RANGE) {
    /* msb and lsb by Range (arg_lsb is ignored) */
    struct RRange *range = mrb_range_ptr(arg_msb);
    if (mrb_fixnum_p(range->edges->beg)) {
      msb = mrb_fixnum(range->edges->beg);
    }
    if (!range->excl &&
        mrb_fixnum_p(range->edges->end)) {
      lsb = mrb_fixnum(range->edges->end);
    }
  }
  else if (mrb_fixnum_p(arg_msb) && mrb_nil_p(arg_lsb)) {
    /* msb only by fixnum */
    msb = lsb = mrb_fixnum(arg_msb);
  }
  else if (mrb_fixnum_p(arg_msb) && mrb_fixnum_p(arg_lsb)) {
    /* msb and lsb by fixnum */
    msb = mrb_fixnum(arg_msb);
    lsb = mrb_fixnum(arg_lsb);
  }

  if (src_data->refs == 0) {
    msb += src_data->lsb;
    lsb += src_data->lsb;
  }

  if (msb > src_data->msb || lsb < src_data->lsb || msb < lsb) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid range");
  }

  new_data = (struct gpio_data *)mrb_calloc(mrb, 1, sizeof(*src_data));
  memcpy(new_data, src_data, sizeof(*src_data));
  new_data->owner = src_data->owner;
  new_data->refs = 0;
  new_data->msb = msb;
  new_data->lsb = lsb;
  new_data->reg = src_data->reg;
  new_data->mask = ((1u << (msb - lsb + 1)) - 1) << lsb;
  ++new_data->owner->refs;

  return pio_wrap(mrb, mrb_obj_class(mrb, self), new_data);
}

static mrb_value
gpio_is_output_disabled(mrb_state *mrb, mrb_value self)
{
  const char *error = "cannot use is_output_disabled for bus signal";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if ((data->msb > data->lsb) || (error = (*data->get_outen)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  if ((value & data->mask) == 0) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpio_is_output_enabled(mrb_state *mrb, mrb_value self)
{
  const char *error = "cannot use is_output_enabled for bus signal";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if ((data->msb > data->lsb) || (error = (*data->get_outen)(data, &value))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  if ((value & data->mask) == data->mask) {
    return mrb_true_value();
  }
  else {
    return mrb_false_value();
  }
}

static mrb_value
gpio_value_set(mrb_state *mrb, mrb_value self)
{
#if defined(MRB_WORD_BOXING)
  const mrb_int max_width = (MRB_INT_BIT - MRB_FIXNUM_SHIFT);
#else
  const mrb_int max_width = (MRB_INT_BIT);
#endif
  const char *error = "width is too large to treat as mrb_int";
  uint32_t value;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  mrb_get_args(mrb, "i", &value);
  value <<= data->lsb;
  value &= data->mask;
  if (((data->msb - data->lsb + 1) > max_width) ||
      (error = (*data->set_value)(data, value)) ||
      (error = (*data->clr_value)(data, value ^ data->mask))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_assert(mrb_state *mrb, mrb_value self)
{
  const char *error;
  uint32_t set;
  uint32_t clr;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  clr = data->owner->polarity & data->mask;
  set = clr ^ data->mask;
  if (((set != 0) && (error = (*data->set_value)(data, set))) ||
      ((clr != 0) && (error = (*data->clr_value)(data, clr)))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_high(mrb_state *mrb, mrb_value self)
{
  const char *error;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (error = (*data->set_value)(data, data->mask)) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_low(mrb_state *mrb, mrb_value self)
{
  const char *error;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (error = (*data->clr_value)(data, data->mask)) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_negate(mrb_state *mrb, mrb_value self)
{
  const char *error;
  uint32_t set;
  uint32_t clr;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  set = data->owner->polarity & data->mask;
  clr = set ^ data->mask;
  if (((set != 0) && (error = (*data->set_value)(data, set))) ||
      ((clr != 0) && (error = (*data->clr_value)(data, clr)))) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_output_disable(mrb_state *mrb, mrb_value self)
{
  const char *error;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (error = (*data->clr_outen)(data, data->mask)) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_output_enable(mrb_state *mrb, mrb_value self)
{
  const char *error;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (error = (*data->set_outen)(data, data->mask)) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
gpio_toggle(mrb_state *mrb, mrb_value self)
{
  const char *error;
  struct gpio_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_gpio_type, struct gpio_data);
  if (error = (*data->tgl_value)(data, data->mask)) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

void
hardware_gpio_init(mrb_state *mrb, struct RClass *mod)
{
  struct RClass *cls;

  //----------------------------------------------------------------
  // Hardware.GPI
  //
  cls = mrb_define_class_under(mrb, mod, "GPI", mrb->object_class);
  MRB_SET_INSTANCE_TT(cls, MRB_TT_DATA);

  // Public methods for properties
  mrb_define_method(mrb, cls, "is_active_high"    , gpi_is_active_high    , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_active_low"     , gpi_is_active_low     , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_asserted"       , gpi_is_asserted       , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_high"           , gpi_is_high           , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_low"            , gpi_is_low            , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_negated"        , gpi_is_negated        , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_output_disabled", gpi_is_output_disabled, MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_output_enabled" , gpi_is_output_enabled , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "value"             , gpi_value             , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "width"             , gpi_width             , MRB_ARGS_NONE());

  // Public methods
  mrb_define_method(mrb, cls, "active_high"       , gpi_active_high       , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "active_low"        , gpi_active_low        , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "slice"             , gpi_slice             , MRB_ARGS_ARG(1,1));

  // Aliases
  mrb_define_alias (mrb, cls, "active_high?"      , "is_active_high");
  mrb_define_alias (mrb, cls, "active_low?"       , "is_active_low");
  mrb_define_alias (mrb, cls, "asserted?"         , "is_asserted");
  mrb_define_alias (mrb, cls, "cleared?"          , "is_low");
  mrb_define_alias (mrb, cls, "high?"             , "is_high");
  mrb_define_alias (mrb, cls, "is_cleared"        , "is_low");
  mrb_define_alias (mrb, cls, "is_off"            , "is_negated");
  mrb_define_alias (mrb, cls, "is_on"             , "is_asserted");
  mrb_define_alias (mrb, cls, "is_set"            , "is_high");
  mrb_define_alias (mrb, cls, "low?"              , "is_low");
  mrb_define_alias (mrb, cls, "negated?"          , "is_negated");
  mrb_define_alias (mrb, cls, "off?"              , "is_negated");
  mrb_define_alias (mrb, cls, "on?"               , "is_asserted");
  mrb_define_alias (mrb, cls, "output_disabled?"  , "is_output_disabled");
  mrb_define_alias (mrb, cls, "output_enabled?"   , "is_output_enabled");
  mrb_define_alias (mrb, cls, "set?"              , "is_high");
  mrb_define_alias (mrb, cls, "[]"                , "slice");

  //----------------------------------------------------------------
  // Hardware.GPIO
  //
  cls = mrb_define_class_under(mrb, mod, "GPIO", cls);

  // Public methods for properties
  mrb_define_method(mrb, cls, "is_output_disabled", gpio_is_output_disabled , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_output_enabled" , gpio_is_output_enabled  , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "value="            , gpio_value_set          , MRB_ARGS_REQ(1));

  // Public methods
  mrb_define_method(mrb, cls, "assert"            , gpio_assert           , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "high"              , gpio_high             , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "low"               , gpio_low              , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "negate"            , gpio_negate           , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "output_disable"    , gpio_output_disable   , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "output_enable"     , gpio_output_enable    , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "toggle"            , gpio_toggle           , MRB_ARGS_NONE());

  // Aliases
  mrb_define_alias (mrb, cls, "disable_output"    , "output_disable");
  mrb_define_alias (mrb, cls, "enable_output"     , "output_enable");
  mrb_define_alias (mrb, cls, "clear"             , "low");
  mrb_define_alias (mrb, cls, "set"               , "high");
  mrb_define_alias (mrb, cls, "off"               , "negate");
  mrb_define_alias (mrb, cls, "on"                , "assert");
}

void
hardware_gpio_final(mrb_state *mrb)
{
  /* do nothing */
}

/* vim: set et sts=2 sw=2: */
