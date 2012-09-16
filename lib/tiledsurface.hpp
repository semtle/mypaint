/* This file is part of MyPaint.
 * Copyright (C) 2008-2011 by Martin Renold <martinxyz@gmx.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <mypaint-tiled-surface.h>

// Used by pythontiledsurface.c,
// needs to be defined here so that swig and the Python bindings will find it
#define TILE_SIZE 64
#define MAX_MIPMAP_LEVEL 4

// Implementation of tiled surface backend
#include "pythontiledsurface.c"

// Interface class, wrapping the backend the way MyPaint wants to use it
class TiledSurface : public Surface {
  // the Python half of this class is in tiledsurface.py

public:
  TiledSurface(PyObject * self_) {
      c_surface = mypaint_python_tiled_surface_new(self_);
      tile_request_in_progress = false;
  }

  ~TiledSurface() {
      mypaint_surface_destroy((MyPaintSurface *)c_surface);
  }

  void set_symmetry_state(bool active, float center_x) {
    mypaint_tiled_surface_set_symmetry_state((MyPaintTiledSurface *)c_surface, active, center_x);
  }

  void begin_atomic() {
      mypaint_surface_begin_atomic((MyPaintSurface *)c_surface);
  }
  void end_atomic() {
      mypaint_surface_end_atomic((MyPaintSurface *)c_surface);
  }

  uint16_t * get_tile_memory(int tx, int ty, bool readonly) {

      // Finish previous request
      if (tile_request_in_progress) {
          mypaint_tiled_surface_tile_request_end((MyPaintTiledSurface *)c_surface, &tile_request);
          tile_request_in_progress = false;
      }

      // Start current request
      mypaint_tiled_surface_tile_request_init(&tile_request,
                                              tx, ty, readonly);

      mypaint_tiled_surface_tile_request_start((MyPaintTiledSurface *)c_surface, &tile_request);
      tile_request_in_progress = true;

      return tile_request.buffer;
  }

  // returns true if the surface was modified
  bool draw_dab (float x, float y, 
                 float radius, 
                 float color_r, float color_g, float color_b,
                 float opaque, float hardness = 0.5,
                 float color_a = 1.0,
                 float aspect_ratio = 1.0, float angle = 0.0,
                 float lock_alpha = 0.0,
                 float colorize = 0.0,
                 int recursing = 0 // used for symmetry, internal use only
                 ) {

    return mypaint_surface_draw_dab((MyPaintSurface *)c_surface, x, y, radius, color_r, color_g, color_b,
                             opaque, hardness, color_a, aspect_ratio, angle,
                             lock_alpha, colorize);
  }

  void get_color (float x, float y, 
                  float radius, 
                  float * color_r, float * color_g, float * color_b, float * color_a
                  ) {
    mypaint_surface_get_color((MyPaintSurface *)c_surface, x, y, radius, color_r, color_g, color_b, color_a);
  }

  float get_alpha (float x, float y, float radius) {
      return mypaint_surface_get_alpha((MyPaintSurface *)c_surface, x, y, radius);
  }

  MyPaintSurface *get_surface_interface() {
    return (MyPaintSurface*)c_surface;
  }

private:
    MyPaintPythonTiledSurface *c_surface;
    MyPaintTiledSurfaceTileRequestData tile_request;
    bool tile_request_in_progress;
};

static PyObject *
get_module(char *name)
{
    PyObject *pName = PyString_FromString(name);
    PyObject *pModule = PyImport_Import(pName);
    Py_DECREF(pName);

    if (pModule != NULL) {

    }
    else {
        PyErr_Print();
        fprintf(stderr, "Failed to load \"%s\"\n", name);
        return NULL;
    }
    return pModule;
}

static PyObject *
new_py_tiled_surface(PyObject *pModule)
{
    PyObject *pFunc = PyObject_GetAttrString(pModule, "new_surface");

    assert(pFunc && PyCallable_Check(pFunc));

    PyObject *pArgs = PyTuple_New(0);
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);

    return pValue;
}

extern "C" {

MyPaintSurface *
mypaint_python_surface_factory(gpointer user_data)
{
    PyObject *module = get_module("tiledsurface");
    PyObject *instance = new_py_tiled_surface(module);
    // Py_DECREF(module);

    swig_type_info *info = SWIG_TypeQuery("TiledSurface *");
    TiledSurface *surf;
    if (SWIG_ConvertPtr(instance, (void **)&surf, info, SWIG_POINTER_EXCEPTION) == -1) {
        return NULL;
    }
    MyPaintSurface *interface = surf->get_surface_interface();

    // Py_DECREF(instance);

    return interface;
}

}
