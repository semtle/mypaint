// gtk stock code - left gtk prefix to use the pygtk wrapper-generator easier
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <math.h>
#include "gtkmybrush.h"
#include "helpers.h"
#include "brush_dab.h" 
;  // ; needed

void
gtk_my_brush_set_base_value (GtkMyBrush * b, int id, float value)
{
  g_assert (id >= 0 && id < BRUSH_SETTINGS_COUNT);
  b->settings[id].base_value = value;
}

void gtk_my_brush_set_mapping (GtkMyBrush * b, int id, int input, int index, float value)
{
  int i;
  g_assert (id >= 0 && id < BRUSH_SETTINGS_COUNT);
  g_assert (input >= 0 && input < INPUT_COUNT);
  g_assert (index >= 0 && index < 8);

  Mapping * m = b->settings[id].mapping[input];
  if (!m) {
    m = b->settings[id].mapping[input] = g_malloc(sizeof(Mapping));
    for (i=0; i<4; i++) {
      m->xvalues[i] = 0;
      m->yvalues[i] = 0;
    }
  }
  if (index % 2 == 0) {
    m->xvalues[index/2] = value;
  } else {
    m->yvalues[index/2] = value;
  }
}

void gtk_my_brush_remove_mapping (GtkMyBrush * b, int id, int input)
{
  g_assert (id >= 0 && id < BRUSH_SETTINGS_COUNT);
  g_assert (input >= 0 && input < INPUT_COUNT);

  g_free(b->settings[id].mapping[input]);
  b->settings[id].mapping[input] = NULL;
}

void
gtk_my_brush_set_color (GtkMyBrush * b, int red, int green, int blue)
{
  g_assert (red >= 0 && red <= 255);
  g_assert (green >= 0 && green <= 255);
  g_assert (blue >= 0 && blue <= 255);
  b->color[0] = red;
  b->color[1] = green;
  b->color[2] = blue;
}


static void gtk_my_brush_class_init    (GtkMyBrushClass *klass);
static void gtk_my_brush_init          (GtkMyBrush      *b);
static void gtk_my_brush_finalize (GObject *object);

static gpointer parent_class;

GType
gtk_my_brush_get_type (void)
{
  static GType my_brush_type = 0;

  if (!my_brush_type)
    {
      static const GTypeInfo my_brush_info =
      {
	sizeof (GtkMyBrushClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) gtk_my_brush_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (GtkMyBrush),
	0,		/* n_preallocs */
	(GInstanceInitFunc) gtk_my_brush_init,
      };

      my_brush_type =
	g_type_register_static (G_TYPE_OBJECT, "GtkMyBrush",
				&my_brush_info, 0);
    }

  return my_brush_type;
}

static void
gtk_my_brush_class_init (GtkMyBrushClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  parent_class = g_type_class_peek_parent (class);
  gobject_class->finalize = gtk_my_brush_finalize;
}

static void
gtk_my_brush_init (GtkMyBrush *b)
{
  int i, j;
  b->queue_draw_widget = NULL;
  for (i=0; i<BRUSH_SETTINGS_COUNT; i++) {
    for (j=0; j<INPUT_COUNT; j++) {
      b->settings[i].mapping[j] = NULL;
    }
  }
  // defaults will be set from python
}

static void
gtk_my_brush_finalize (GObject *object)
{
  GtkMyBrush * b;
  int i, j;
  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_MY_BRUSH (object));
  b = GTK_MY_BRUSH (object);
  for (i=0; i<BRUSH_SETTINGS_COUNT; i++) {
    for (j=0; j<INPUT_COUNT; j++) {
      g_free(b->settings[i].mapping[j]);
    }
  }
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkMyBrush*
gtk_my_brush_new (void)
{
  g_print ("This gets never called... but is needed. Strange.\n");
  return g_object_new (GTK_TYPE_MY_BRUSH, NULL);
}

float exp_decay (float T_const, float t)
{
  // FIXME: think about whether the argument make mathematical sense
  if (T_const <= 0.001) {
    return 1.0;
  } else {
    return 1.0-exp(- t / T_const);
  }
}

void brush_reset (GtkMyBrush * b)
{
  b->time = 0;
}

// high-level part of before each dab
void brush_prepare_and_draw_dab (GtkMyBrush * b, Surface * s)
{
  float x, y, radius_log, opaque;
  float speed;
  int i;
  gint color[3];
  int color_is_hsv;
  float pressure;
  float settings[BRUSH_SETTINGS_COUNT];

  // FIXME: does happen (interpolation problem?)
  if (b->pressure < 0.0) b->pressure = 0.0;
  if (b->pressure > 1.0) b->pressure = 1.0;
  g_assert (b->pressure >= 0.0 && b->pressure <= 1.0);
  pressure = b->pressure; // could distort it here

  for (i=0; i<BRUSH_SETTINGS_COUNT; i++) {
    settings[i] = b->settings[i].base_value;

    // FIXME: each setting might depend on any input however, some
    // inputs need the value of a setting to be updated. Using the old
    // value here, this results in a small lag that would be avoidable
    // in most cases. -- however if one of the inputs jumps between
    // two strokes (speed for example) this might be fatal.

    // ==> the inputs should be updated above, and the settings needed
    // to do so should have been cached from the last step... argh,
    // I'm complaining about bugs in code that is not even written.

    // TODO: add function mapping code here.
  }


  { // slow position 2
    float fac = exp_decay (settings[BRUSH_POSITION_T2], 0.4);
    b->x_slow += (b->x - b->x_slow) * fac;
    b->y_slow += (b->y - b->y_slow) * fac;
    x = b->x_slow;
    y = b->y_slow;
  }

  //x = b->x; y = b->y;
  radius_log = settings[BRUSH_RADIUS_LOGARITHMIC];
  opaque = settings[BRUSH_OPAQUE];

  speed = sqrt(sqr(b->dx) + sqr(b->dy))/b->dtime;

  { // slow speed
    float fac = exp_decay (settings[BRUSH_OBS__SPEED_SLOWNESS] * 0.01, 0.1 * b->dtime);
    b->dx_slow += (b->dx - b->dx_slow) * fac;
    b->dy_slow += (b->dy - b->dy_slow) * fac;

    fac = exp_decay (settings[BRUSH_OBS__SPEEDABS_SLOWNESS], 0.1 * b->dtime);
    b->obs__speedabs_slow += (speed - b->obs__speedabs_slow) * fac;

    fac = exp_decay (settings[BRUSH_RBS__SPEEDABS_SLOWNESS] * 0.001, 0.1 * b->dtime);
    b->rbs__speedabs_slow += (speed - b->rbs__speedabs_slow) * fac;
  }

  // TODO: think about it: is this setting enough?
  //opaque *= (settings[BRUSH_OPAQUE_BY_PRESSURE] * b->pressure + (1-settings[BRUSH_OPAQUE_BY_PRESSURE]));
  opaque *= settings[BRUSH_OPAQUE_BY_PRESSURE] * pressure;
  if (opaque >= 1.0) opaque = 1.0;
  //b->radius = 2.0 + sqrt(sqrt(speed));
  radius_log += pressure * settings[BRUSH_RADIUS_BY_PRESSURE];

  if (settings[BRUSH_RADIUS_BY_RANDOM]) {
    radius_log += (g_random_double () - 0.5) * settings[BRUSH_RADIUS_BY_RANDOM];
  }

  if (settings[BRUSH_RADIUS_BY_SPEED]) {
    radius_log += 0.001 * b->rbs__speedabs_slow * settings[BRUSH_RADIUS_BY_SPEED];
  }

  if (settings[BRUSH_OFFSET_BY_RANDOM]) {
    x += gauss_noise () * settings[BRUSH_OFFSET_BY_RANDOM];
    y += gauss_noise () * settings[BRUSH_OFFSET_BY_RANDOM];
  }

  if (settings[BRUSH_OFFSET_BY_SPEED]) {
    //x += b->dx_slow * b->offset_by_speed; // * radius?
    //y += b->dy_slow * b->offset_by_speed;
    x += b->dx_slow * 0.01*b->obs__speedabs_slow * settings[BRUSH_OFFSET_BY_SPEED]; // * radius?
    y += b->dy_slow * 0.01*b->obs__speedabs_slow * settings[BRUSH_OFFSET_BY_SPEED];
  }

  // color part
  
  for (i=0; i<3; i++) color[i] = b->color[i];
  color_is_hsv = 0;

  if (settings[BRUSH_ADAPT_COLOR_FROM_IMAGE]) {
    int px, py;
    guchar *rgb;
    float v = settings[BRUSH_ADAPT_COLOR_FROM_IMAGE];
    px = ROUND(x);
    py = ROUND(y);
    if (px < 0) px = 0;
    if (py < 0) py = 0;
    if (px > s->w-1) px = s->w - 1;
    if (py > s->h-1) py = s->h - 1;
    rgb = PixelXY(s, px, py);
    for (i=0; i<3; i++) {
      color[i] = ROUND((1.0-v)*color[i] + v*rgb[i]);
      if (color[i] < 0) color[i] = 0;
      if (color[i] > 255) color[i] = 255;
      b->color[i] = color[i];
    }
  }

  if (settings[BRUSH_COLOR_VALUE_BY_RANDOM] ||
      settings[BRUSH_COLOR_VALUE_BY_PRESSURE] ||
      settings[BRUSH_COLOR_SATURATION_BY_RANDOM] ||
      settings[BRUSH_COLOR_SATURATION_BY_PRESSURE] ||
      settings[BRUSH_COLOR_HUE_BY_RANDOM] ||
      settings[BRUSH_COLOR_HUE_BY_PRESSURE]) {
    color_is_hsv = 1;
    gimp_rgb_to_hsv_int (color + 0, color + 1, color + 2);
  }
  

  if (settings[BRUSH_COLOR_HUE_BY_RANDOM]) {
    g_assert (color_is_hsv);
    color[0] = ROUND ((float)(color[0]) + gauss_noise () * settings[BRUSH_COLOR_HUE_BY_RANDOM] * 64.0);
  }
  if (settings[BRUSH_COLOR_HUE_BY_PRESSURE]) {
    g_assert (color_is_hsv);
    color[0] += ROUND (pressure * settings[BRUSH_COLOR_HUE_BY_PRESSURE] * 128.0);
  }
  if (settings[BRUSH_COLOR_SATURATION_BY_RANDOM]) {
    g_assert (color_is_hsv);
    color[1] = ROUND ((float)(color[1]) + gauss_noise () * settings[BRUSH_COLOR_SATURATION_BY_RANDOM] * 64.0);
  }
  if (settings[BRUSH_COLOR_SATURATION_BY_PRESSURE]) {
    g_assert (color_is_hsv);
    color[1] += ROUND (pressure * settings[BRUSH_COLOR_SATURATION_BY_PRESSURE] * 128.0);
  }
  if (settings[BRUSH_COLOR_VALUE_BY_RANDOM]) {
    g_assert (color_is_hsv);
    color[2] = ROUND ((float)(color[2]) + gauss_noise () * settings[BRUSH_COLOR_VALUE_BY_RANDOM] * 64.0);
  }
  if (settings[BRUSH_COLOR_VALUE_BY_PRESSURE]) {
    g_assert (color_is_hsv);
    color[2] += ROUND (pressure * settings[BRUSH_COLOR_VALUE_BY_PRESSURE] * 128.0);
  }

  { // final calculations
    float radius;
    guchar c[3];
    radius = expf(radius_log);
    
    g_assert(radius > 0);
    //FIXME: performance problem acutally depending on CPU
    if (radius > 100) radius = 100;
    g_assert(opaque >= 0);
    g_assert(opaque <= 1);
    
    // used for interpolation later
    b->actual_radius = radius;
    
    if (color_is_hsv) {
      while (color[0] < 0) color[0] += 360;
      while (color[0] > 360) color[0] -= 360;
      if (color[1] < 0) color[1] = 0;
      if (color[1] > 255) color[1] = 255;
      if (color[2] < 0) color[2] = 0;
      if (color[2] > 255) color[2] = 255;
      gimp_hsv_to_rgb_int (color + 0, color + 1, color + 2);
    }
    for (i=0; i<3; i++) c[i] = color[i];

    draw_brush_dab (s, b->queue_draw_widget,
                    x, y, radius, opaque, settings[BRUSH_HARDNESS], c, settings[BRUSH_SATURATION_SLOWDOWN]);
  }
}

float brush_count_dabs_to (GtkMyBrush * b, float x, float y, float pressure, float time)
{
  float dx, dy, dt;
  float res1, res2, res3;
  float dist;
  if (b->actual_radius == 0) b->actual_radius = expf(b->settings[BRUSH_RADIUS_LOGARITHMIC].base_value);
  if (b->actual_radius < 0.5) b->actual_radius = 0.5;
  if (b->actual_radius > 500.0) b->actual_radius = 500.0;
  dx = x - b->x;
  dy = y - b->y;
  //dp = pressure - b->pressure; // Not useful?
  dt = time - b->time;

  dist = sqrtf (dx*dx + dy*dy);
  res1 = dist / b->actual_radius * b->settings[BRUSH_DABS_PER_ACTUAL_RADIUS].base_value /*FIXME?*/;
  res2 = dist / expf(b->settings[BRUSH_RADIUS_LOGARITHMIC].base_value) * b->settings[BRUSH_DABS_PER_BASIC_RADIUS].base_value /*FIXME?*/;
  res3 = dt * b->settings[BRUSH_DABS_PER_SECOND].base_value /*FIXME?*/;
  return res1 + res2 + res3;
}

void brush_stroke_to (GtkMyBrush * b, Surface * s, float x, float y, float pressure, float time)
{
  float dist;
  if (time <= b->time) return;

  if (b->time == 0 || time - b->time > 5) {
    // reset
    b->x = x;
    b->y = y;
    b->pressure = pressure;
    b->time = time;

    b->last_time = b->time;
    b->x_slow = b->x;
    b->y_slow = b->y;
    b->dx_slow = 0.0;
    b->dy_slow = 0.0;
    return;
  }

  { // calculate the actual "virtual" cursor position
    float fac = exp_decay (b->settings[BRUSH_POSITION_T].base_value, 100.0*(time - b->last_time));
    x = b->x + (x - b->x) * fac;
    y = b->y + (y - b->y) * fac;
  }
  // draw many (or zero) dabs to the next position
  dist = brush_count_dabs_to (b, x, y, pressure, time);
  //g_print("dist = %f\n", dist);
  // Not going to recalculate dist each step.

  while (dist >= 1.0) {
    { // linear interpolation
      // Inside the loop because outside it produces numerical errors
      // resulting in b->pressure being small negative and such effects.
      float step;
      step = 1 / dist;
      b->dx        = step * (x - b->x);
      b->dy        = step * (y - b->y);
      b->dpressure = step * (pressure - b->pressure);
      b->dtime     = step * (time - b->time);
    }
    
    b->x        += b->dx;
    b->y        += b->dy;
    b->pressure += b->dpressure;
    b->time     += b->dtime;

    dist -= 1.0;
    
    brush_prepare_and_draw_dab (b, s);
  }

  // not equal to b_time now unless dist == 0
  b->last_time = time;
}
