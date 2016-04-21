%define %aioarray_class(TYPE,NAME)
/* #if defined(SWIGPYTHON_BUILTIN) */
/*   %feature("python:slot", "sq_item", functype="ssizeargfunc") NAME::__getitem__; */
/*   %feature("python:slot", "sq_ass_item", functype="ssizeobjargproc") NAME::__setitem__; */

#if defined(SWIGPYTHON)
%exception NAME::__getitem__ {
    if ( arg2 > arg1->_size - 1 || arg2 < 0 ) {
        PyErr_SetString(PyExc_IndexError,"Index out of range");
        return NULL;
    }
    $action
}

%exception NAME::__setitem__ {
    if ( arg2 > arg1->_size - 1 || arg2 < 0 ) {
        PyErr_SetString(PyExc_IndexError,"Index out of range");
        return NULL;
    }
    $action
}
#endif

%inline %{
typedef struct {
    TYPE *el;
    int _size;
} NAME;
%}

%extend NAME {

  NAME(size_t nelements) {
      NAME *arr = (NAME*)malloc(sizeof(NAME));
      arr->el = (TYPE *)calloc(nelements, sizeof(TYPE));
      arr->_size = (int)nelements;
      return arr;
  }

  ~NAME() {
      free(self->el);
      free(self);
  }
  
  TYPE __getitem__(int index) {
      return self->el[index];
  }

  void __setitem__(int index, TYPE value) {
      self->el[index] = value;
  }

  TYPE * cast() {
      return self->el;
  }

  static NAME *frompointer(TYPE *t) {
      return (NAME *)t;
  }

}

%extend NAME {
    const char *__repr__() {
        static char buf[BUFSIZ];
        snprintf(buf,BUFSIZ,"TYPE [%d]", self->_size );
        return buf;
    }
}


%types(NAME = TYPE);


%enddef




