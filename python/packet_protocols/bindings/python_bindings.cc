/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <pybind11/pybind11.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace py = pybind11;

// Headers for binding functions
/**************************************/
// The following comment block is used for
// gr_modtool to insert function prototypes
// Please do not delete
/**************************************/
// BINDING_FUNCTION_PROTOTYPES(
    void bind_ax25_encoder(py::module& m);
    void bind_ax25_decoder(py::module& m);
    void bind_fx25_encoder(py::module& m);
    void bind_fx25_decoder(py::module& m);
    void bind_il2p_encoder(py::module& m);
    void bind_il2p_decoder(py::module& m);
    void bind_kiss_tnc(py::module& m);
// ) END BINDING_FUNCTION_PROTOTYPES


// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
void* init_numpy()
{
    import_array();
    return NULL;
}

PYBIND11_MODULE(packet_protocols_python, m)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    // Allow access to base block methods
    py::module::import("gnuradio.gr");

    /**************************************/
    // The following comment block is used for
    // gr_modtool to insert binding function calls
    // Please do not delete
    /**************************************/
    // BINDING_FUNCTION_CALLS(
    bind_ax25_encoder(m);
    bind_ax25_decoder(m);
    bind_fx25_encoder(m);
    bind_fx25_decoder(m);
    bind_il2p_encoder(m);
    bind_il2p_decoder(m);
    bind_kiss_tnc(m);
    // ) END BINDING_FUNCTION_CALLS
}