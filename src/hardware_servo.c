#include "hardware_servo.h"

const struct mrb_data_type hardware_servo_type = {"Hardware.Servo", mrb_free};

static mrb_value
servo_channel(mrb_state *mrb, mrb_value self)
{
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  return mrb_fixnum_value(data->channel);
}

static mrb_value
servo_is_enabled(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int enabled;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  if ((error = (*data->get_enabled)(data, &enabled)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return (enabled ? mrb_true_value() : mrb_false_value());
}

static mrb_value
servo_is_disabled(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int enabled;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  if ((error = (*data->get_enabled)(data, &enabled)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return (enabled ? mrb_false_value() : mrb_true_value());
}

static mrb_value
servo_position(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int raw;
  int position;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  if ((error = (*data->get_position)(data, &raw)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  position = (raw - data->calib_min) * 255 / (data->calib_max - data->calib_min);
  return mrb_fixnum_value(position);
}

static mrb_value
servo_position_set(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int position;
  int raw;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  mrb_get_args(mrb, "i", &position);
  if ((position < 0) || (position > 255)) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid range");
  }
  raw = position * (data->calib_max - data->calib_min) / 255 + data->calib_min;
  if ((error = (*data->let_position)(data, raw)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
servo_raw_position(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int raw;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  if ((error = (*data->get_position)(data, &raw)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return mrb_fixnum_value(position);
}

static mrb_value
servo_raw_position_set(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int raw;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  mrb_get_args(mrb, "i", &raw);
  if ((raw < 0) || (raw > 255)) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid range");
  }
  if ((error = (*data->let_position)(data, raw)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return self;
}

static mrb_value
servo_calib_min(mrb_state *mrb, mrb_value self)
{
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  return mrb_fixnum_value(data->calib_min);
}

static mrb_value
servo_calib_min_set(mrb_state *mrb, mrb_value self)
{
  int value;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  mrb_get_args(mrb, "i", &position);
  if ((value < 0) || (value > 255) || (value > data->calib_max)) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid range");
  }
  data->calib_min = (uint8_t)value;
  return self;
}

static mrb_value
servo_calib_max(mrb_state *mrb, mrb_value self)
{
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  return mrb_fixnum_value(data->calib_max);
}

static mrb_value
servo_calib_max_set(mrb_state *mrb, mrb_value self)
{
  int value;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  mrb_get_args(mrb, "i", &position);
  if ((value < 0) || (value > 255) || (value < data->calib_min)) {
    mrb_raisef(mrb, E_ARGUMENT_ERROR, "invalid range");
  }
  data->calib_max = (uint8_t)value;
  return self;
}

static mrb_value
servo_configure(mrb_state *mrb, mrb_value self)
{
  int pin;
  extern void hardware_pin_configure(mrb_state *mrb, ...);
  hardware_pin_configure(mrb, "<pin", &pin, NULL);
  return self;
}

static mrb_value
servo_enable(mrb_state *mrb, mrb_value self)
{
  const char *error;
  int position;
  struct servo_data *data;
  data = DATA_GET_PTR(mrb, self, &hardware_servo_type, struct servo_data);
  if ((error = (*data->get_position)(data, &position)) != NULL) {
    mrb_raisef(mrb, E_TYPE_ERROR, error);
  }
  return mrb_fixnum_value(position);
}

void
hardware_servo_init(mrb_state *mrb, struct RClass *mod)
{
  struct RClass *cls;

  //----------------------------------------------------------------
  // Hardware.Servo
  //
  cls = mrb_define_class_under(mrb, mod, "Servo", mrb->object_class);
  MRB_SET_INSTANCE_TT(cls, MRB_TT_DATA);

  // Public methods for properties
  mrb_define_method(mrb, cls, "channel"       , servo_channel         , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "is_disabled"   , servo_is_disabled     , MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cls, "is_enabled"    , servo_is_enabled      , MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cls, "position"      , servo_position        , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "position="     , servo_position_set    , MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cls, "raw_position"  , servo_raw_position    , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "raw_position=" , servo_raw_position_set, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cls, "calib_min"     , servo_calib_min       , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "calib_min="    , servo_calib_min_set   , MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cls, "calib_max"     , servo_calib_max       , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "calib_max="    , servo_calib_max_set   , MRB_ARGS_REQ(1));

  // Public methods
  mrb_define_method(mrb, cls, "configure"   , servo_configure   , MRB_ARGS_REQ(1));
  mrb_define_method(mrb, cls, "disable"     , servo_disable     , MRB_ARGS_NONE());
  mrb_define_method(mrb, cls, "enable"      , servo_enable      , MRB_ARGS_NONE());

  // Aliases
  mrb_define_alias (mrb, cls, "enabled?"    , "is_enabled");
  mrb_define_alias (mrb, cls, "disabled?"   , "is_disabled");
  mrb_define_alias (mrb, cls, "move"        , "position=");
}

void
hardware_servo_final(mrb_state *mrb)
{
  /* do nothing */
}

/* vim: set et sts=2 sw=2: */
