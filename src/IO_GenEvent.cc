/**
 *  @file IO_GenEvent.cc
 *  @brief Implementation of \b class HepMC3::IO_GenEvent
 *
 */
#include "HepMC3/IO_GenEvent.h"
#include "HepMC3/GenEvent.h"
#include "HepMC3/GenParticle.h"
#include "HepMC3/GenVertex.h"
#include "HepMC3/Log.h"

#include <fstream>
#include <iostream>

#include <boost/range/iterator_range.hpp>
#include <boost/foreach.hpp>
using std::endl;

namespace HepMC3 {

void IO_GenEvent::write_event(const GenEvent &evt) {
    if ( m_file.rdstate() ) return;
    if ( m_mode != std::ios::out ) {
        ERROR( "IO_GenEvent: attempting to write to input file" )
        return;
    }

    m_file << "E " << evt.event_number()
           << " "  << evt.vertices_count()
           << " "  << evt.particles_count()
           << endl;

    // Print vertices
    for( unsigned int i=1; i<=evt.vertices_count(); ++i ) {
        write_vertex( evt, evt.get_vertex(-i));
    }
}

bool IO_GenEvent::fill_next_event(GenEvent &evt) {
    if ( m_file.rdstate() ) return 0;
    if ( m_mode != std::ios::in ) {
        ERROR( "IO_GenEvent: attempting to read from output file" )
        return 0;
    }

    WARNING( "IO_GenEvent: Reading not implemented (yet)" )

    return 1;
}

void IO_GenEvent::write_vertex(const GenEvent &evt, const GenVertex &v) {

    // Write all incoming particles
    BOOST_FOREACH( int p_barcode, v.particles_in() ) {
        write_particle( evt.get_particle(p_barcode) );
    }

    m_file << "V " << v.barcode()
           << " [";

    if(v.particles_in().size()) {
        BOOST_FOREACH( int p_barcode, boost::make_iterator_range(v.particles_in().begin(), v.particles_in().end()-1) ) {
            m_file << p_barcode <<",";
        }
        m_file << v.particles_in().back();
    }

    m_file << "] ";

    m_file << "@ 0 0 0 0";
    m_file << endl;

    // Write outgoing particles without their end vertex
    BOOST_FOREACH( int p_barcode, v.particles_out() ) {
        const GenParticle &p = evt.get_particle(p_barcode);
        if( !p.end_vertex() ) write_particle(p);
    }
}

void IO_GenEvent::write_particle(const GenParticle &p) {
    std::ios_base::fmtflags orig = m_file.flags();
    std::streamsize prec = m_file.precision();
    m_file.setf(std::ios::scientific, std::ios::floatfield);
    m_file.precision(m_precision);

    m_file << "P "<< p.barcode()
           << " " << p.production_vertex()
           << " " << p.pdg_id()
           << " " << p.momentum().px()
           << " " << p.momentum().py()
           << " " << p.momentum().pz()
           << " " << p.momentum().e()
           << " " << p.generated_mass()
           << " " << p.status()
           << endl;

    m_file.flags(orig);
    m_file.precision(prec);
}

} // namespace HepMC3
