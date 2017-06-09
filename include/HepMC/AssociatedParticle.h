
// -*- C++ -*-
//
// This file is part of HepMC
// Copyright (C) 2014-2015 The HepMC collaboration (see AUTHORS for details)
//
#ifndef  HEPMC_AssociatedParticle_H
#define  HEPMC_AssociatedParticle_H
/**
 *  @file AssociatedParticle.h
 *  @brief Definition of \b class AssociatedParticle,
 *
 *  @class HepMC::AssociatedParticle @brief Attribute class allowing
 *  eg. a GenParticle to refer to another GenParticle.

 *  @ingroup attributes
 *
 */

#include "HepMC/Attribute.h"
#include "HepMC/GenParticle.h"

namespace HepMC {

/**
 *  @class HepMC::IntAttribute
 *  @brief Attribute that holds an Integer implemented as an int
 *
 *  @ingroup attributes
 */
class AssociatedParticle : public IntAttribute {
public:

    /** @brief Default constructor */
    AssociatedParticle() {}

    /** @brief Constructor initializing attribute value */
    AssociatedParticle(GenParticlePtr p)
        : IntAttribute(p->id()), m_associated(p) {}

    /** @brief Implementation of Attribute::from_string */
    bool from_string(const string &att) {
        IntAttribute::from_string(att);
        if ( associatedId() > int(event()->particles().size()) ||
             associatedId() <= 0  ) return false;
        m_associated = event()->particles()[associatedId() -1];
        return true;
    }

    /** @brief get id of the associated particle. */
    int associatedId() const {
	return value();
    }

    /** @Brief get a pointer to the associated particle. */
    GenParticlePtr associated() const {
        return m_associated;
    }

    /** @brief set the value associated to this Attribute. */
    void set_associated(GenParticlePtr p) {
        IntAttribute::set_value(p->id());
        m_associated = p;
    }

private:
    
    GenParticlePtr m_associated; ///< The associated particle.

};

} // namespace HepMC

#endif
