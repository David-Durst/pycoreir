#include "coreir/coreir-python.hpp"


CoreIR::Type* CoreIR::TypeGenFromPython::createType(Context* c, Values values) {
  Type* type_ptr = NULL;
  wchar_t python_home[] = PYTHON_HOME;
  Py_SetPythonHome(python_home);
  Py_Initialize();
  PyObject *py_module = PyImport_ImportModule(moduleName.c_str());
  if (py_module != NULL) {
    Py_INCREF(py_module);
    PyObject *py_typeGenFunc = PyObject_GetAttrString(py_module, functionName.c_str());
    if (py_typeGenFunc && PyCallable_Check(py_typeGenFunc)) {
      Py_INCREF(py_typeGenFunc);
      int size = values.size();
      char** names = (char**) malloc(size * sizeof(char*));
      Value** values_ptrs = (Value**) malloc(sizeof(Value*) * size);
      int count = 0;
      for (auto element : values) {
          std::size_t name_length = element.first.size();
          names[count] = (char*) malloc(sizeof(char) * name_length + 1);
          memcpy(names[count], element.first.c_str(), name_length + 1);
          values_ptrs[count] = element.second;
          count++;
      }
      char signature[] = "llli";
      PyObject* value_object = PyObject_CallFunction(py_typeGenFunc, signature,
              (void *) c, (void *) names, (void *) values_ptrs, size);
      if (!value_object) {
        if (PyErr_Occurred()) PyErr_Print();
        std::cerr << "Error calling typegen function " << functionName << std::endl;
      } else {
        type_ptr = (Type *) PyLong_AsVoidPtr(value_object);
        Py_DECREF(value_object);
      }
      for (uint i = 0; i < values.size(); i++) {
        free(names[i]);
      }
      free(names);
      free(values_ptrs);
      Py_DECREF(py_typeGenFunc);
    } else {
      if (PyErr_Occurred()) PyErr_Print();
      std::cerr << "Cannot find function " << functionName << std::endl;
    }
    Py_DECREF(py_module);
  } else {
    PyErr_Print();
    std::cerr << "Failed to load " << moduleName << std::endl;
    ASSERT(0, "Failed to load module");
  }

  Py_Finalize();

  // FIXME: Can we free char** names and Value** values_ptrs because
  // they are no longer used since the interpreter's been finalized?
  // Currently they will be cleaned up eventually by the context, but
  // if we can free here that should reduce memory consumption
  return type_ptr;
}
