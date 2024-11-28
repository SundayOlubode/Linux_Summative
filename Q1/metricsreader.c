#include <Python.h>

static PyObject *read_metrics(PyObject *self, PyObject *args)
{
	const char *filename;
	if (!PyArg_ParseTuple(args, "s", &filename))
		return NULL;

	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		PyErr_SetString(PyExc_FileNotFoundError, "Could not open metrics file");
		return NULL;
	}

	char buffer[256];
	fgets(buffer, sizeof(buffer), file);

	PyObject *timestamps = PyList_New(0);
	// PyObject *cpu_usagee = PyList_New(0);
	PyObject *cpu_usage = PyList_New(0);
	PyObject *memory_usage = PyList_New(0);
	PyObject *network_received = PyList_New(0);
	PyObject *network_transmitted = PyList_New(0);

	while (fgets(buffer, sizeof(buffer), file))
	{
		char timestamp[9];
		float cpu, mem;
		long recv, trans;

		sscanf(buffer, "%s\t%f\t%f\t%ld\t%ld",
		       timestamp, &cpu, &mem, &recv, &trans);

		PyList_Append(timestamps, PyUnicode_FromString(timestamp));
		PyList_Append(cpu_usage, PyFloat_FromDouble(cpu));
		PyList_Append(memory_usage, PyFloat_FromDouble(mem));
		PyList_Append(network_received, PyLong_FromLong(recv));
		PyList_Append(network_transmitted, PyLong_FromLong(trans));
	}

	fclose(file);

	PyObject *result = PyTuple_New(5);
	PyTuple_SetItem(result, 0, timestamps);
	PyTuple_SetItem(result, 1, cpu_usage);
	PyTuple_SetItem(result, 2, memory_usage);
	PyTuple_SetItem(result, 3, network_received);
	PyTuple_SetItem(result, 4, network_transmitted);

	return result;
}

static PyMethodDef MetricsReaderMethods[] = {
    {"read_metrics", read_metrics, METH_VARARGS, "Read metrics from file"},
    {NULL, NULL, 0, NULL}};

static struct PyModuleDef metricsreader = {
    PyModuleDef_HEAD_INIT,
    "metricsreader",
    NULL,
    -1,
    MetricsReaderMethods};

PyMODINIT_FUNC PyInit_metricsreader(void)
{
	return PyModule_Create(&metricsreader);
}