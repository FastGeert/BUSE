/*
 * pythonbd - example memory-based block device using BUSE
 * Copyright (C) 2016 Geert Audenaert
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Python.h>

#include "buse.h"

static int xmpl_debug = 1;

PyObject *xmp_read_impl, *xmp_write_impl, *xmp_disc_impl, *xmp_flush_impl, *xmp_trim_impl, *xmp_size_impl;

int xmp_read_set = 0;
int xmp_write_set = 0;
int xmp_disc_set = 0;
int xmp_flush_set = 0;
int xmp_trim_set = 0;
int xmp_size_set = 0;

static int xmp_read(void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
  if (*(int *)userdata)
    fprintf(stderr, "R - %lu, %u\n", offset, len);
  // def read(length, offset):
  //     return 'dddd'.encode() # Needs to return a bytes object
  PyObject *pArgs = PyTuple_New(2);
  PyTuple_SetItem(pArgs, 0, PyLong_FromUnsignedLong(len));
  PyTuple_SetItem(pArgs, 1, PyLong_FromUnsignedLongLong(offset));
  PyObject *pValue = PyObject_CallObject(xmp_read_impl, pArgs);
  Py_DECREF(pArgs);
  if ( pValue == NULL ) {
    fprintf(stderr, "R Python implementation raised an exception.\n");
    PyErr_Print();
    return 1;
  }
  if ( ! PyBytes_Check(pValue) ) {
    fprintf(stderr, "R Python implementation did not return a bytes object.\n");
    Py_DECREF(pValue);
    return 1;
  }
  if ( PyBytes_Size(pValue) != len ) {
    fprintf(stderr, "R Python implementation did not return expected amount (%u) of bytes.\n", len);
    Py_DECREF(pValue);
    return 1;
  }
  char *data_returned = PyBytes_AsString(pValue);
  memcpy(buf, data_returned, len);
  Py_DECREF(pValue);
  return 0;
}

static int xmp_write(const void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
  if (*(int *)userdata)
    fprintf(stderr, "W - %lu, %u\n", offset, len);
  // def write(data, offset):
  //     pass
  PyObject *buf_bytes = PyBytes_FromStringAndSize(buf, len);
  if ( buf_bytes == NULL ) {
    fprintf(stderr, "W Failed to convert buffer into bytes object.\n");
    PyErr_Print();
    return 1;
  }
  PyObject *pArgs = PyTuple_New(2);
  PyTuple_SetItem(pArgs, 0, buf_bytes);
  PyTuple_SetItem(pArgs, 1, PyLong_FromUnsignedLongLong(offset));
  PyObject *pValue = PyObject_CallObject(xmp_write_impl, pArgs);
  Py_DECREF(pArgs);
  Py_DECREF(buf_bytes);
  if ( pValue == NULL ) {
    fprintf(stderr, "W Python implementation raised an exception.\n");
    PyErr_Print();
    return 1;
  }
  Py_DECREF(pValue);
  return 0;
}

static void xmp_disc(void *userdata)
{
  (void)(userdata);
  fprintf(stderr, "Received a disconnect request.\n");
  // def disc():
  //     pass
  PyObject *pArgs = PyTuple_New(0);
  PyObject *pValue = PyObject_CallObject(xmp_disc_impl, pArgs);
  Py_DECREF(pArgs);
  if ( pValue == NULL ) {
    fprintf(stderr, "D Python implementation raised an exception.\n");
    PyErr_Print();
  }
  Py_DECREF(pValue);
}

static int xmp_flush(void *userdata)
{
  (void)(userdata);
  fprintf(stderr, "Received a flush request.\n");
  // def flush():
  //     pass
  PyObject *pArgs = PyTuple_New(0);
  PyObject *pValue = PyObject_CallObject(xmp_flush_impl, pArgs);
  Py_DECREF(pArgs);
  if ( pValue == NULL ) {
    fprintf(stderr, "F Python implementation raised an exception.\n");
    PyErr_Print();
    return 1;
  }
  Py_DECREF(pValue);
  return 0;
}

static int xmp_trim(u_int64_t from, u_int32_t len, void *userdata)
{
  (void)(userdata);
  fprintf(stderr, "T - %lu, %u\n", from, len);
  // def trim(offset, length):
  //     pass
  PyObject *pArgs = PyTuple_New(2);
  PyTuple_SetItem(pArgs, 0, PyLong_FromUnsignedLongLong(from));
  PyTuple_SetItem(pArgs, 1, PyLong_FromUnsignedLong(len));
  PyObject *pValue = PyObject_CallObject(xmp_trim_impl, pArgs);
  Py_DECREF(pArgs);
  if ( pValue == NULL ) {
    fprintf(stderr, "T Python implementation raised an exception.\n");
    PyErr_Print();
    return 1;
  }
  Py_DECREF(pValue);
  return 0;
}

static struct buse_operations get_buse_operations(void)
{
  struct buse_operations aop = {
    .read = xmp_read,
    .write = xmp_write,
    .disc = xmp_disc,
    .flush = xmp_flush,
    .trim = xmp_trim,
  };

  // def size():
  //     pass
  PyObject *pArgs = PyTuple_New(0);
  PyObject *pValue = PyObject_CallObject(xmp_size_impl, pArgs);
  Py_DECREF(pArgs);
  if ( pValue == NULL ) {
    fprintf(stderr, "S Python implementation raised an exception.\n");
    PyErr_Print();
    return aop;
  }
  if ( PyLong_Check(pValue) != 0 ) {
    u_int64_t size = PyLong_AsUnsignedLongLong(pValue);
    Py_DECREF(pValue);
    aop.size = size;
    return aop;
  } else {
    fprintf(stderr, "S Python implementation did not return an integer.\n");
    Py_DECREF(pValue);
    return aop;
  }
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    fprintf(stderr,
        "Usage:\n"
        "  %s /dev/nbd0\n"
        "Don't forget to load nbd kernel module (`modprobe nbd`) and\n"
        "run example from root.\n", argv[0]);
    return 1;
  }

  PyObject *pName, *pModule;
  Py_Initialize();
  PyRun_SimpleString("import sys;sys.path.append('.')\n");
  pName = PyUnicode_DecodeFSDefault(argv[2]);
  pModule = PyImport_Import(pName);
  Py_DECREF(pName);
  if (pModule == NULL) {
    PyErr_Print();
    fprintf(stderr, "Failed to load \"%s\"\n", argv[2]);
    Py_Finalize();
    return 1;
  }

  PyObject *list = PyObject_Dir(pModule);
  for (int i=0,max=PyList_Size(list); i<max; i++) { 
    PyObject *attr_name = PyList_GetItem(list, i);
    PyObject *attr_value = PyObject_GetAttr(pModule, attr_name);
    int match = 0;    

    if (PyCallable_Check(attr_value) != 0) { 
      if ( PyUnicode_CompareWithASCIIString(attr_name, "read") == 0 ) {
        xmp_read_impl = attr_value;
        xmp_read_set = 1;
        match = 1;
      } else if ( PyUnicode_CompareWithASCIIString(attr_name, "write") == 0 ) {
        xmp_write_impl = attr_value;
        xmp_write_set = 1;
        match = 1;
      } else if ( PyUnicode_CompareWithASCIIString(attr_name, "disc") == 0 ) {
        xmp_disc_impl = attr_value;
        xmp_disc_set = 1;
        match = 1;
      } else if ( PyUnicode_CompareWithASCIIString(attr_name, "flush") == 0 ) {
        xmp_flush_impl = attr_value;
        xmp_flush_set = 1;
        match = 1;
      } else if ( PyUnicode_CompareWithASCIIString(attr_name, "trim") == 0 ) {
        xmp_trim_impl = attr_value;
        xmp_trim_set = 1;
        match = 1;
      } else if ( PyUnicode_CompareWithASCIIString(attr_name, "size") == 0 ) {
        xmp_size_impl = attr_value;
        xmp_size_set = 1;
        match = 1;
      }

    }

    if ( match == 0 ) {
      Py_DECREF(attr_name);
      if ( attr_value != NULL ) Py_DECREF(attr_value);
    }
  }
  Py_DECREF(list);
  if ( ! ( xmp_read_set == 1 && xmp_write_set == 1 && 
           xmp_disc_set == 1 && xmp_flush_set == 1 && 
           xmp_trim_set == 1 && xmp_size_set ) ) {
    if ( xmp_read_set == 1) Py_DECREF(xmp_read_impl); else fprintf(stderr, "read function not found\n");
    if ( xmp_write_set == 1 ) Py_DECREF(xmp_write_impl); else fprintf(stderr, "write function not found\n");
    if ( xmp_disc_set == 1 ) Py_DECREF(xmp_disc_impl); else fprintf(stderr, "disc function not found\n");
    if ( xmp_flush_set == 1 ) Py_DECREF(xmp_flush_impl); else fprintf(stderr, "flush function not found\n");
    if ( xmp_trim_set == 1 ) Py_DECREF(xmp_trim_impl); else fprintf(stderr, "trim function not found\n");
    if ( xmp_size_set == 1 ) Py_DECREF(xmp_size_impl); else fprintf(stderr, "size function not found\n");
    fprintf(stderr, "Not all required functions are present in \"%s\"\n", argv[2]);
    Py_DECREF(pModule);
    Py_Finalize();
    return 1;    
  }

  struct buse_operations aop = get_buse_operations();

  int retval = 0;
  if ( aop.size != 0) {
     retval = buse_main(argv[1], &aop, (void *)&xmpl_debug);
  } else
    retval = 1;
  Py_DECREF(xmp_read_impl);
  Py_DECREF(xmp_write_impl);
  Py_DECREF(xmp_disc_impl);
  Py_DECREF(xmp_flush_impl);
  Py_DECREF(xmp_trim_impl);
  Py_DECREF(xmp_size_impl);
  Py_DECREF(pModule);
  Py_Finalize();
  return retval;
}
