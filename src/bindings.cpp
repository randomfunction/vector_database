#include<pybind11/pybind11.h>
#include<pybind11/stl.h>
#include"../include/database.hpp"

namespace py=pybind11;

PYBIND11_MODULE(custom_vectordb,m){
    py::enum_<MetricType>(m,"MetricType")
        .value("L2",MetricType::L2)
        .value("DOT",MetricType::DOT)
        .value("COSINE",MetricType::COSINE)
        .export_values();
        
    py::class_<SearchResult>(m,"SearchResult")
        .def_readwrite("uuid",&SearchResult::uuid)
        .def_readwrite("distance",&SearchResult::dist)
        .def_readwrite("metadata",&SearchResult::metadata);
        
    py::class_<Engine>(m,"Engine")
        .def(py::init<MetricType>(),py::arg("m")=MetricType::L2)
        .def("reserve",&Engine::reserve)
        .def("insert",&Engine::insert)
        // Release the GIL for the duration of the native search so concurrent
        // Python-side requests actually run in parallel through the engine's
        // shared (read) lock instead of serializing on the interpreter lock.
        // Safe here: search takes only a shared_lock and touches no Python
        // objects; pybind converts the args/return value with the GIL held.
        .def("search",&Engine::search,py::call_guard<py::gil_scoped_release>())
        .def("save_to_file",&Engine::save_to_file)
        .def("load_from_file",&Engine::load_from_file);
}
