// -*- C++ -*-
//
// This file is part of HepMC
// Copyright (C) 2014-2019 The HepMC collaboration (see AUTHORS for details)
//
///
/// @file GenEvent.h
/// @brief Definition of \b class GenEvent
///
#ifndef HEPMC3_GENEVENT_H
#define HEPMC3_GENEVENT_H

#include "HepMC3/Units.h"
#include "HepMC3/GenParticle_fwd.h"
#include "HepMC3/GenVertex_fwd.h"
#include "HepMC3/GenPdfInfo_fwd.h"
#include "HepMC3/GenHeavyIon_fwd.h"
#include "HepMC3/GenCrossSection_fwd.h"

#if !defined(__CINT__)
#include "HepMC3/Errors.h"
#include "HepMC3/GenHeavyIon.h"
#include "HepMC3/GenPdfInfo.h"
#include "HepMC3/GenCrossSection.h"
#include "HepMC3/GenRunInfo.h"
#include <mutex>
#endif // __CINT__

#ifdef HEPMC3_ROOTIO
class TBuffer;
#endif


namespace HepMC {

struct GenEventData;

/// @brief Stores event-related information
///
/// Manages event-related information.
/// Contains lists of GenParticle and GenVertex objects
class GenEvent {

public:

    /// @brief Event constructor without a run
    GenEvent(Units::MomentumUnit momentum_unit = Units::GEV,
             Units::LengthUnit length_unit = Units::MM);

    #if !defined(__CINT__)

    /// @brief Constructor with associated run
    GenEvent(shared_ptr<GenRunInfo> run,
             Units::MomentumUnit momentum_unit = Units::GEV,
             Units::LengthUnit length_unit = Units::MM);

    /// @brief Copy constructor 
     GenEvent(const GenEvent&);

    /// @brief Destructor 
    ~GenEvent();

    /// @brief Assignment operator
     GenEvent& operator=(const GenEvent&);
 
    /// @name Particle and vertex access
    //@{

    /// @brief Get list of particles (const)
    std::vector<ConstGenParticlePtr> particles() const;
    /// @brief Get list of vertices (const)
    std::vector<ConstGenVertexPtr> vertices() const;


    /// @brief Get/set list of particles (non-const)
    const std::vector<GenParticlePtr>& particles() { return m_particles; }
    /// @brief Get/set list of vertices (non-const)
    const std::vector<GenVertexPtr>& vertices() { return m_vertices; }

    //@}


    /// @name Event weights
    //@{

    /// Get event weight values as a vector
    const std::vector<double>& weights() const { return m_weights; }
    /// Get event weights as a vector (non-const)
    std::vector<double>& weights() { return m_weights; }
    /// Get event weight accessed by index (or the canonical/first one if there is no argument)
    /// @note It's the user's responsibility to ensure that the given index exists!
    double weight(const size_t& index=0) const { return weights().at(index); }
    /// Get event weight accessed by weight name
    /// @note Requires there to be an attached GenRunInfo, otherwise will throw an exception
    /// @note It's the user's responsibility to ensure that the given name exists!
    double weight(const std::string& name) const {
      if (!run_info()) throw WeightError("GenEvent::weight(str): named access to event weights requires the event to have a GenRunInfo");
      return weight(run_info()->weight_index(name));
    }
    /// Get event weight accessed by weight name
    /// @note Requires there to be an attached GenRunInfo, otherwise will throw an exception
    /// @note It's the user's responsibility to ensure that the given name exists!
    double& weight(const std::string& name) {
      if (!run_info()) throw WeightError("GenEvent::weight(str): named access to event weights requires the event to have a GenRunInfo");
      int pos=run_info()->weight_index(name);
      if (pos<0) throw WeightError("GenEvent::weight(str): no weight with given name in this run");
      return m_weights[pos];
    }
    /// Get event weight names, if there are some
    /// @note Requires there to be an attached GenRunInfo with registered weight names, otherwise will throw an exception
    const std::vector<std::string>& weight_names(const std::string& /*name*/) const {
      if (!run_info()) throw WeightError("GenEvent::weight_names(): access to event weight names requires the event to have a GenRunInfo");
      const std::vector<std::string>& weightnames = run_info()->weight_names();
      if (weightnames.empty()) throw WeightError("GenEvent::weight_names(): no event weight names are registered for this run");
      return weightnames;
    }

    //@}


    /// @name Auxiliary info and event metadata
    //@{

    /// @brief Get a pointer to the the GenRunInfo object.
    shared_ptr<GenRunInfo> run_info() const {
      return m_run_info;
    }
    /// @brief Set the GenRunInfo object by smart pointer.
    void set_run_info(shared_ptr<GenRunInfo> run) {
      m_run_info = run;
      if ( run && !run->weight_names().empty() )
        m_weights.resize(run->weight_names().size(), 1.0);
    }

    /// @brief Get event number
    int  event_number() const { return m_event_number; }
    /// @brief Set event number
    void set_event_number(const int& num) { m_event_number = num; }

    /// @brief Get momentum unit
    const Units::MomentumUnit& momentum_unit() const { return m_momentum_unit; }
    /// @brief Get length unit
    const Units::LengthUnit& length_unit() const { return m_length_unit; }
    /// @brief Change event units
    /// Converts event from current units to new ones
    void set_units( Units::MomentumUnit new_momentum_unit, Units::LengthUnit new_length_unit);

    #ifndef HEPMC3_NO_DEPRECATED
    /// Converts event from current units to new ones (compatibility name)
    void use_units( Units::MomentumUnit new_momentum_unit, Units::LengthUnit new_length_unit) {
      set_units(new_momentum_unit, new_length_unit);
    }
    #endif

    /// @brief Get heavy ion generator additional information
    ConstGenHeavyIonPtr heavy_ion() const { return attribute<GenHeavyIon>("GenHeavyIon"); }
    /// @brief Set heavy ion generator additional information
    void set_heavy_ion(const GenHeavyIonPtr &hi) { add_attribute("GenHeavyIon",hi); }

    /// @brief Get PDF information
//FIXME! 
    const GenPdfInfoPtr pdf_info() const { return attribute<GenPdfInfo>("GenPdfInfo"); }
    /// @brief Set PDF information
    void set_pdf_info(const GenPdfInfoPtr &pi) { add_attribute("GenPdfInfo",pi); }

    /// @brief Get cross-section information
    const GenCrossSectionPtr cross_section() const { return attribute<GenCrossSection>("GenCrossSection"); }
    /// @brief Set cross-section information
    void set_cross_section(const GenCrossSectionPtr &cs) { add_attribute("GenCrossSection",cs); }

    //@}


    /// @name Event position
    //@{

    /// Vertex representing the overall event position
    const FourVector& event_pos() const;

    /// @brief Vector of beam particles
    std::vector<ConstGenParticlePtr> beams() const;

    /// @brief Vector of beam particles
    const std::vector<GenParticlePtr> & beams();
  
    /// @brief Shift position of all vertices in the event by @a delta
    void shift_position_by( const FourVector & delta );

    /// @brief Shift position of all vertices in the event to @a op
    void shift_position_to( const FourVector & newpos ) {
      const FourVector delta = newpos - event_pos();
      shift_position_by(delta);
    }

    //@}


    /// @name Additional attributes
    //@{
    /// @brief Add event attribute to event
    ///
    /// This will overwrite existing attribute if an attribute
    /// with the same name is present
    void add_attribute(const string &name, const shared_ptr<Attribute> &att,  const int& id = 0) {
      std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
      if ( att ) {
        m_attributes[name][id] = att;
        att->m_event = this;
        if ( id > 0 && id <= int(particles().size()) )
          att->m_particle = particles()[id - 1];
        if ( id < 0 && -id <= int(vertices().size()) )
          att->m_vertex = vertices()[-id - 1];
      }
   }

    /// @brief Remove attribute
    void remove_attribute(const string &name,  const int& id = 0);

    /// @brief Get attribute of type T
    template<class T>
    shared_ptr<T> attribute(const string &name,  const int& id = 0) const;

    /// @brief Get attribute of any type as string
    string attribute_as_string(const string &name,  const int& id = 0) const;

    /// @brief Get list of attribute names
    std::vector<string> attribute_names( const int& id = 0) const;

    /// @brief Get a copy of the list of attributes
    /// @todo To avoid thread issues, this is returns a copy. Better solution may be needed.
std::map< string, std::map<int, shared_ptr<Attribute> > > attributes() const {
       std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
       return m_attributes;
    }

    //@}


    /// @name Particle and vertex modification
    //@{

    /// @brief Add particle
    void add_particle( GenParticlePtr p );

    /// @brief Add vertex
    void add_vertex( GenVertexPtr v );

    /// @brief Remove particle from the event
    ///
    /// This function  will remove whole sub-tree starting from this particle
    /// if it is the only incoming particle of this vertex.
    /// It will also production vertex of this particle if this vertex
    /// has no more outgoing particles
    void remove_particle( GenParticlePtr v );

    /// @brief Remove a set of particles
    ///
    /// This function follows rules of GenEvent::remove_particle to remove
    /// a list of particles from the event.
    void remove_particles( std::vector<GenParticlePtr> v );

    /// @brief Remove vertex from the event
    ///
    /// This will remove all sub-trees of all outgoing particles of this vertex
    /// @todo Optimize. Currently each particle/vertex is erased separately
    void remove_vertex( GenVertexPtr v );

    /// @brief Add whole tree in topological order
    ///
    /// This function will find the beam particles (particles
    /// that have no production vertices or their production vertices
    /// have no particles) and will add the whole decay tree starting from
    /// these particles.
    ///
    /// @note Any particles on this list that do not belong to the tree
    ///       will be ignored.
    void add_tree( const std::vector<GenParticlePtr> &particles );

    /// @brief Reserve memory for particles and vertices
    ///
    /// Helps optimize event creation when size of the event is known beforehand
    void reserve(const size_t& particles, const size_t& vertices = 0);

    /// @brief Remove contents of this event
    void clear();

    //@}

    /// @name Deprecated functionality
    //@{

    #ifndef HEPMC3_NO_DEPRECATED

    /// @brief Add particle by raw pointer
    /// @deprecated Use GenEvent::add_particle( const GenParticlePtr& ) instead
    HEPMC3_DEPRECATED("Use GenParticlePtr instead of GenParticle*")
    void add_particle( GenParticle *p );

    /// @brief Add vertex by raw pointer
    /// @deprecated Use GenEvent::add_vertex( const GenVertexPtr& ) instead
    HEPMC3_DEPRECATED("Use GenVertexPtr instead of GenVertex*")
    void add_vertex  ( GenVertex *v );

    /// @deprecated Backward compatibility iterators
    typedef std::vector<GenParticlePtr>::iterator  particle_iterator;
    /// @deprecated Backward compatibility iterators
    typedef std::vector<GenParticlePtr>::const_iterator  particle_const_iterator;

    /// @deprecated Backward compatibility iterators
    typedef std::vector<GenVertexPtr>::iterator vertex_iterator;
    /// @deprecated Backward compatibility iterators
    typedef std::vector<GenVertexPtr>::const_iterator  vertex_const_iterator;

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container particles() instead")
    particle_iterator       particles_begin()       { return m_particles.begin(); }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container particles() instead")
    particle_iterator       particles_end()         { return m_particles.end();   }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container particles() instead")
    particle_const_iterator particles_begin() const { return m_particles.begin(); }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container particles() instead")
    particle_const_iterator particles_end()   const { return m_particles.end();   }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container vertices() instead")
    vertex_iterator         vertices_begin()        { return m_vertices.begin();  }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container vertices() instead")
    vertex_iterator         vertices_end()          { return m_vertices.end();    }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container vertices() instead")
    vertex_const_iterator   vertices_begin()  const { return m_vertices.begin();  }

    /// @deprecated Backward compatibility iterators
    HEPMC3_DEPRECATED("Iterate over std container vertices() instead")
    vertex_const_iterator   vertices_end()    const { return m_vertices.end();    }

    /// @deprecated Backward compatibility
    HEPMC3_DEPRECATED("Use particles().size() instead")
    int  particles_size()  const { return m_particles.size();  }

    /// @deprecated Backward compatibility
    HEPMC3_DEPRECATED("Use particles().empty() instead")
    bool particles_empty() const { return m_particles.empty(); }

    /// @deprecated Backward compatibility
    HEPMC3_DEPRECATED("Use vertices().size() instead")
    int  vertices_size()   const { return m_vertices.size();   }

    /// @deprecated Backward compatibility
    HEPMC3_DEPRECATED("Use vertices().empty() instead")
    bool vertices_empty()  const { return m_vertices.empty();  }

    /// @brief Test to see if we have exactly two particles in event_pos() vertex
    /// @deprecated Backward compatibility
    HEPMC3_DEPRECATED("Use beams() to access beam particles")
    bool valid_beam_particles() const;

    /// @brief Get first two particles of the event_pos() vertex
    /// @deprecated Backward compatibility
    HEPMC3_DEPRECATED("Use beams() to access beam particles")
    std::pair<GenParticlePtr,GenParticlePtr> beam_particles() const;

    /// @brief Set incoming beam particles
    /// @deprecated Backward compatibility
    /// @todo Set/require status = 4 at the same time?
    HEPMC3_DEPRECATED("instead add particle without production vertex to the event")
    void set_beam_particles(GenParticlePtr p1, GenParticlePtr p2);

    /// @brief Set incoming beam particles
    /// @deprecated Backward compatibility
    /// @todo Set/require status = 4 at the same time?
    HEPMC3_DEPRECATED("instead add particle without production vertex to the event")
    void set_beam_particles(const std::pair<GenParticlePtr,GenParticlePtr>& p);

    #endif

    //@}

    #endif // __CINT__


    /// @name Methods to fill GenEventData and to read it back
    //@{

    /// @brief Fill GenEventData object
    void write_data(GenEventData &data) const;

    /// @brief Fill GenEvent based on GenEventData
    void read_data(const GenEventData &data);

    #ifdef HEPMC3_ROOTIO
    /// @brief ROOT I/O streamer
    void Streamer(TBuffer &b);
    //@}
    #endif

private:

    /// @name Fields
    //@{

    #if !defined(__CINT__)

    /// List of particles
    std::vector<GenParticlePtr> m_particles;
    /// List of vertices
    std::vector<GenVertexPtr> m_vertices;

    /// Event number
    /// @todo Move to attributes?
    int m_event_number;

    /// Event weights
    std::vector<double> m_weights;

    /// Momentum unit
    Units::MomentumUnit m_momentum_unit;
    /// Length unit
    Units::LengthUnit m_length_unit;

    /// The root vertex is stored outside the normal vertices list to block user access to it
    GenVertexPtr m_rootvertex;

    /// Global run information.
    shared_ptr<GenRunInfo> m_run_info;

    /// @brief Map of event, particle and vertex attributes
    ///
    /// Keys are name and ID (0 = event, <0 = vertex, >0 = particle)
    mutable std::map< string, std::map<int, shared_ptr<Attribute> > > m_attributes;

    /// @brief Attribute map key type
    typedef std::map< string, std::map<int, shared_ptr<Attribute> > >::value_type att_key_t;

    /// @brief Attribute map value type
    typedef std::map<int, shared_ptr<Attribute> >::value_type att_val_t;

    /// @breif Mutex lock for the m_attibutes map.
    mutable std::recursive_mutex m_lock_attributes;
    #endif // __CINT__

    //@}

};

#if !defined(__CINT__)
//
// Template methods
//
template<class T>
shared_ptr<T> GenEvent::attribute(const std::string &name,  const int& id) const {
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    std::map< string, std::map<int, shared_ptr<Attribute> > >::iterator i1 =
      m_attributes.find(name);
    if( i1 == m_attributes.end() ) {
        if ( id == 0 && run_info() ) {
            return run_info()->attribute<T>(name);
        }
        return shared_ptr<T>();
    }

    std::map<int, shared_ptr<Attribute> >::iterator i2 = i1->second.find(id);
    if (i2 == i1->second.end() ) return shared_ptr<T>();

    if (!i2->second->is_parsed() ) {

        shared_ptr<T> att = make_shared<T>();
        att->m_event = this;

        if ( id > 0 && id <= int(particles().size()) )
          att->m_particle = m_particles[id - 1];
        if ( id < 0 && -id <= int(vertices().size()) )
          att->m_vertex = m_vertices[-id - 1];
        if ( att->from_string(i2->second->unparsed_string()) &&
             att->init() ) {
            // update map with new pointer
            i2->second = att;
            return att;
        } else {
            return shared_ptr<T>();
        }
    }
    else return dynamic_pointer_cast<T>(i2->second);
}
#endif // __CINT__

} // namespace HepMC
#endif