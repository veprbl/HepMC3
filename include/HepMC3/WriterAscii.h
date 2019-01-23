// -*- C++ -*-
//
// This file is part of HepMC
// Copyright (C) 2014-2019 The HepMC collaboration (see AUTHORS for details)
//
#ifndef HEPMC3_WRITERASCII_H
#define HEPMC3_WRITERASCII_H
///
/// @file  WriterAscii.h
/// @brief Definition of class \b WriterAscii
///
/// @class HepMC::WriterAscii
/// @brief GenEvent I/O serialization for structured text files
///
/// @ingroup IO
///
#include "HepMC3/Writer.h"
#include "HepMC3/GenEvent.h"
#include "HepMC3/GenRunInfo.h"
#include <string>
#include <fstream>

namespace HepMC {

class WriterAscii : public Writer {
public:

    /// @brief Constructor
    /// @warning If file already exists, it will be cleared before writing
    WriterAscii(const std::string& filename,
        shared_ptr<GenRunInfo> run = shared_ptr<GenRunInfo>());

    /// @brief Constructor from ostream
    WriterAscii(std::ostream& stream,
        shared_ptr<GenRunInfo> run = shared_ptr<GenRunInfo>());

    /// @brief Destructor
    ~WriterAscii();

    /// @brief Write event to file
    ///
    /// @param[in] evt Event to be serialized
    void write_event(const GenEvent& evt);

    /// @brief Write the GenRunInfo object to file.
    void write_run_info();

    /// @brief Return status of the stream
    bool failed() { return (bool)m_file.rdstate(); }

    /// @brief Close file stream
    void close();

    /// @brief Set output precision
    ///
    /// So far available range is [2,24]. Default is 16.
    void set_precision(const int& prec ) {
        if (prec < 2 || prec > 24) return;
        m_precision = prec;
    }
    /// @brief Return output precision
    int precision() const {
        return m_precision;
    }
private:

    /// @name Buffer management
    //@{

    /// @brief Attempts to allocate buffer of the chosen size
    ///
    /// This function can be called manually by the user or will be called
    /// before first read/write operation
    ///
    /// @note If buffer size is too large it will be divided by 2 until it is
    /// small enough for system to allocate
    void allocate_buffer();

    /// @brief Set buffer size (in bytes)
    ///
    /// Default is 256kb. Minimum is 256b.
    /// Size can only be changed before first read/write operation.
    void set_buffer_size(const size_t& size ) {
        if (m_buffer) return;
        if (size < 256) return;
        m_buffer_size = size;
    }

    /// @brief Escape '\' and '\n' characters in string
    std::string escape(const std::string& s)  const;

    /// Inline function flushing buffer to output stream when close to buffer capacity
    void flush();

    /// Inline function forcing flush to the output stream
    void forced_flush();

    //@}


    /// @name Write helpers
    //@{

    /// @brief Inline function for writing strings
    ///
    /// Since strings can be long (maybe even longer than buffer) they have to be dealt
    /// with separately.
    void write_string( const std::string &str );

    /// @brief Write vertex
    ///
    /// Helper routine for writing single vertex to file
    void write_vertex(ConstGenVertexPtr v);

    /// @brief Write particle
    ///
    /// Helper routine for writing single particle to file
    void write_particle(ConstGenParticlePtr p, int second_field);

    //@}

private:

    std::ofstream m_file; //!< Output file
    std::ostream* m_stream; //!< Output stream

    int m_precision; //!< Output precision
    char* m_buffer;  //!< Stream buffer
    char* m_cursor;  //!< Cursor inside stream buffer
    unsigned long m_buffer_size; //!< Buffer size

};


} // namespace HepMC

#endif