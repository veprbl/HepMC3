#ifndef  HEPMC3_IO_BASE_H
#define  HEPMC3_IO_BASE_H
/**
 *  @file  IO_Base.h
 *  @brief Definition of \b class HepMC3::IO_Base
 *
 *  @class HepMC3::IO_Base
 *  @brief Abstract class serving as a base of I/O interfaces
 *
 *  Handles file-related issues leaving the actual
 *  I/O operations to implementations
 *
 *  @date Created       <b> 23th March 2014 </b>
 *  @date Last modified <b> 25th March 2014 </b>
 */
#include <string>
#include <fstream>
#include "HepMC3/GenEvent.h"
namespace HepMC3 {

class IO_Base {
//
// Constructors
//
public:
    /** Default constructor
     *  @warning Only ios::in or ios::out mode is allowed
     */
    IO_Base(const std::string &filename, std::ios::openmode mode);
    /** Default destructor.
     *  Closes I/O stream (if opened)
     */
    ~IO_Base();
//
// Functions
//
public:
    /** Write event to file */
    virtual void write_event(const GenEvent *evt) = 0;

    /** Get event from file */
    virtual bool fill_next_event(GenEvent *evt)   = 0;

    /** Close the I/O stream */
    void close();

//
// Accessors
//
public:
    std::ios::iostate rdstate()  { return m_file.rdstate(); } //!< Get IO stream error state

//
// Fields
//
protected:
    std::fstream        m_file; //!< I/O stream
    std::ios::openmode  m_mode; //!< I/O stream mode
    int m_io_error_state;       //!< Error state @todo Change to use m_file.rdstate() instead
};

} // namespace HepMC3

#endif
