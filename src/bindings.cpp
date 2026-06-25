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
        .def_readwrite("distance",&SearchResult::distance)
        .def_readwrite("metadata",&SearchResult::metadata);
        
    py::class_<Engine>(m,"Engine")
        .def(py::init<MetricType>(),py::arg("m")=MetricType::L2)
        .def("reserve",&Engine::reserve)
        .def("insert",&Engine::insert)
        .def("search",&Engine::search)
        .def("save_to_file",&Engine::save_to_file)
        .def("load_from_file",&Engine::load_from_file);
}
