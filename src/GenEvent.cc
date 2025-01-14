// -*- C++ -*-
//
// This file is part of HepMC
// Copyright (C) 2014-2023 The HepMC collaboration (see AUTHORS for details)
//
/**
 *  @file GenEvent.cc
 *  @brief Implementation of \b class GenEvent
 *
 */
#include <algorithm> // sort
#include <deque>

#include "HepMC3/Data/GenEventData.h"
#include "HepMC3/GenEvent.h"
#include "HepMC3/GenParticle.h"
#include "HepMC3/GenVertex.h"


namespace HepMC3 {

GenEvent::GenEvent(Units::MomentumUnit mu,
                   Units::LengthUnit lu)
    : m_momentum_unit(mu), m_length_unit(lu), //m_weights(std::vector<double>(1, 1.0)),//Prevent from  different number of weights and names
      m_rootvertex(std::make_shared<GenVertex>()) {}


GenEvent::GenEvent(std::shared_ptr<GenRunInfo> run,
                   Units::MomentumUnit mu,
                   Units::LengthUnit lu)
    : m_momentum_unit(mu), m_length_unit(lu),  //m_weights(std::vector<double>(1, 1.0)),//Prevent from  different number of weights and names
      m_rootvertex(std::make_shared<GenVertex>()),
      m_run_info(run) {
    if ( run && !run->weight_names().empty() ) {
        m_weights = std::vector<double>(run->weight_names().size(), 1.0);
    }
}

const std::vector<ConstGenParticlePtr>& GenEvent::particles() const {
    return *(reinterpret_cast<const std::vector<ConstGenParticlePtr>*>(&m_particles));
}

const std::vector<ConstGenVertexPtr>& GenEvent::vertices() const {
    return *(reinterpret_cast<const std::vector<ConstGenVertexPtr>*>(&m_vertices));
}


void GenEvent::add_particle(GenParticlePtr p) {
    if ( !p || p->in_event() ) return;

    m_particles.emplace_back(p);

    p->m_event = this;
    p->m_id = particles().size();

    // Particles without production vertex are added to the root vertex
    if ( !p->production_vertex() ) {
        m_rootvertex->add_particle_out(p);
    }
}


GenEvent::GenEvent(const GenEvent&e) {
    if (this != &e)
    {
        std::lock(m_lock_attributes, e.m_lock_attributes);
        std::lock_guard<std::recursive_mutex> lhs_lk(m_lock_attributes, std::adopt_lock);
        std::lock_guard<std::recursive_mutex> rhs_lk(e.m_lock_attributes, std::adopt_lock);
        GenEventData tdata;
        e.write_data(tdata);
        read_data(tdata);
    }
}

GenEvent::~GenEvent() {
    for ( auto attm = m_attributes.begin(); attm != m_attributes.end(); ++attm) {
        for ( auto att = attm->second.begin(); att != attm->second.end(); ++att) { if (att->second) att->second->m_event = nullptr;}
    }
    for  ( auto v = m_vertices.begin(); v != m_vertices.end(); ++v ) if (*v)  if ((*v)->m_event == this) (*v)->m_event = nullptr;
    for  ( auto p = m_particles.begin(); p != m_particles.end(); ++p ) if (*p)  if ((*p)->m_event == this)  (*p)->m_event = nullptr;
}

GenEvent& GenEvent::operator=(const GenEvent& e) {
    if (this != &e)
    {
        std::lock(m_lock_attributes, e.m_lock_attributes);
        std::lock_guard<std::recursive_mutex> lhs_lk(m_lock_attributes, std::adopt_lock);
        std::lock_guard<std::recursive_mutex> rhs_lk(e.m_lock_attributes, std::adopt_lock);
        GenEventData tdata;
        e.write_data(tdata);
        read_data(tdata);
    }
    return *this;
}


void GenEvent::add_vertex(GenVertexPtr v) {
    if ( !v|| v->in_event() ) return;
    m_vertices.emplace_back(v);

    v->m_event = this;
    v->m_id = -(int)vertices().size();

    // Add all incoming and outgoing particles and restore their production/end vertices
    for (const auto& p: v->particles_in()) {
        if (!p->in_event()) add_particle(p);
        p->m_end_vertex = v->shared_from_this();
    }

    for (const auto& p: v->particles_out()) {
        if (!p->in_event()) add_particle(p);
        p->m_production_vertex = v;
    }
}


void GenEvent::remove_particle(GenParticlePtr p) {
    if ( !p || p->parent_event() != this ) return;

    HEPMC3_DEBUG(30, "GenEvent::remove_particle - called with particle: " << p->id());
    GenVertexPtr end_vtx = p->end_vertex();
    if ( end_vtx ) {
        end_vtx->remove_particle_in(p);

        // If that was the only incoming particle, remove vertex from the event
        if ( end_vtx->particles_in().empty() )  remove_vertex(end_vtx);
    }

    GenVertexPtr prod_vtx = p->production_vertex();
    if ( prod_vtx ) {
        prod_vtx->remove_particle_out(p);

        // If that was the only outgoing particle, remove vertex from the event
        if ( prod_vtx->particles_out().empty() ) remove_vertex(prod_vtx);
    }

    HEPMC3_DEBUG(30, "GenEvent::remove_particle - erasing particle: " << p->id())

    int idx = p->id();
    auto it = m_particles.erase(m_particles.begin() + idx-1);

    // Remove attributes of this particle
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    for (att_key_t& vt1: m_attributes) {
        auto vt2 = vt1.second.find(idx);
        if (vt2 == vt1.second.end()) continue;
        vt1.second.erase(vt2);
    }

    //
    // Reassign id of attributes with id above this one
    //
    std::vector< std::pair< int, std::shared_ptr<Attribute> > > changed_attributes;

    for (att_key_t& vt1: m_attributes) {
        changed_attributes.clear();

        for (auto vt2 = vt1.second.begin(); vt2 != vt1.second.end(); ++vt2) {
            if ( (*vt2).first > p->id() ) {
                changed_attributes.emplace_back(*vt2);
            }
        }

        for ( const auto& val: changed_attributes ) {
            vt1.second.erase(val.first);
            vt1.second[val.first-1] = val.second;
        }
    }
    // Reassign id of particles with id above this one
    for (; it != m_particles.end(); ++it) {
        --((*it)->m_id);
    }

    // Finally - set parent event and id of this particle to 0
    p->m_event = nullptr;
    p->m_id    = 0;
}

void GenEvent::remove_particles(std::vector<GenParticlePtr> v) {
    std::sort(v.begin(), v.end(), [](const GenParticlePtr& p1, const GenParticlePtr& p2) { return p1->id() > p2->id();});

    for (auto p = v.begin(); p != v.end(); ++p) {
        remove_particle(*p);
    }
}

void GenEvent::remove_vertex(GenVertexPtr v) {
    if ( !v || v->parent_event() != this ) return;

    HEPMC3_DEBUG(30, "GenEvent::remove_vertex   - called with vertex:  " << v->id());
    std::shared_ptr<GenVertex> null_vtx;

    for (const auto& p: v->particles_in()) {
        p->m_end_vertex = std::weak_ptr<GenVertex>();
    }

    for (const auto& p: v->particles_out()) {
        p->m_production_vertex = std::weak_ptr<GenVertex>();

        // recursive delete rest of the tree
        remove_particle(p);
    }

    // Erase this vertex from vertices list
    HEPMC3_DEBUG(30, "GenEvent::remove_vertex   - erasing vertex: " << v->id())

    int idx = -v->id();
    auto it = m_vertices.erase(m_vertices.begin() + idx-1);
    // Remove attributes of this vertex
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    for (att_key_t& vt1: m_attributes) {
        auto vt2 = vt1.second.find(-idx);
        if (vt2 == vt1.second.end()) continue;
        vt1.second.erase(vt2);
    }

    //
    // Reassign id of attributes with id below this one
    //

    std::vector< std::pair< int, std::shared_ptr<Attribute> > > changed_attributes;

    for ( att_key_t& vt1: m_attributes ) {
        changed_attributes.clear();

        for (auto vt2 = vt1.second.begin(); vt2 != vt1.second.end(); ++vt2) {
            if ( (*vt2).first < v->id() ) {
                changed_attributes.emplace_back(*vt2);
            }
        }

        for ( const auto& val: changed_attributes ) {
            vt1.second.erase(val.first);
            vt1.second[val.first+1] = val.second;
        }
    }

    // Reassign id of particles with id above this one
    for (; it != m_vertices.end(); ++it) {
        ++((*it)->m_id);
    }

    // Finally - set parent event and id of this vertex to 0
    v->m_event = nullptr;
    v->m_id    = 0;
}
/// @todo This looks dangerously similar to the recusive event traversel that we forbade in the
///       Core library due to wories about generator dependence
static bool visit_children(std::map<ConstGenVertexPtr, int>  &a, const ConstGenVertexPtr& v)
{
    for (const ConstGenParticlePtr& p: v->particles_out()) {
        if (p->end_vertex())
        {
            if (a[p->end_vertex()] != 0) { return true; }
            a[p->end_vertex()]++;
            if (visit_children(a, p->end_vertex())) return true;
        }
    }
    return false;
}

void GenEvent::add_tree(const std::vector<GenParticlePtr> &parts) {
    m_particles.reserve(m_particles.size() + parts.size());
    m_vertices.reserve(m_vertices.size() + parts.size());
    std::shared_ptr<IntAttribute> existing_hc = attribute<IntAttribute>("cycles");
    bool has_cycles = false;
    std::map<GenVertexPtr, int>  sortingv;
    std::vector<GenVertexPtr> noinv;
    if (existing_hc)     if (existing_hc->value() != 0) has_cycles = true;
    if (!existing_hc)
    {
        for (const GenParticlePtr& p: parts) {
            GenVertexPtr v = p->production_vertex();
            if (v) sortingv[v]=0;
            if ( !v || v->particles_in().empty()) {
                GenVertexPtr v2 = p->end_vertex();
                if (v2) {noinv.emplace_back(v2); sortingv[v2] = 0;}
            }
        }
        for (const GenVertexPtr& v: noinv) {
            std::map<ConstGenVertexPtr, int>  sorting_temp(sortingv.begin(), sortingv.end());
            has_cycles = (has_cycles || visit_children(sorting_temp, v));
        }
    }
    if (has_cycles) {
        add_attribute("cycles", std::make_shared<IntAttribute>(1));
        /* Commented out  as improvemnts allow us to do sorting in other way.
         for ( std::map<GenVertexPtr,int>::iterator vi=sortingv.begin();vi!=sortingv.end();++vi) if ( !vi->first->in_event() ) add_vertex(vi->first);
         return;
         */
    }

    std::deque<GenVertexPtr> sorting;

    // Find all starting vertices (end vertex of particles that have no production vertex)
    for (const auto& p: parts) {
        const GenVertexPtr &v = p->production_vertex();
        if ( !v || v->particles_in().empty() ) {
            const GenVertexPtr &v2 = p->end_vertex();
            if (v2) sorting.emplace_back(v2);
        }
    }

    HEPMC3_DEBUG_CODE_BLOCK(
        unsigned int sorting_loop_count = 0;
        unsigned int max_deque_size     = 0;
    )

    // Add vertices to the event in topological order
    while ( !sorting.empty() ) {
        HEPMC3_DEBUG_CODE_BLOCK(
            if ( sorting.size() > max_deque_size ) max_deque_size = sorting.size();
            ++sorting_loop_count;
        )

            GenVertexPtr &v = sorting.front();

        bool added = false;

        // Add all mothers to the front of the list
        for (const auto& p: v->particles_in() ) {
            GenVertexPtr v2 = p->production_vertex();
            if ( v2 && !v2->in_event() && find(sorting.begin(), sorting.end(), v2) == sorting.end() ) {
                sorting.push_front(v2);
                added = true;
            }
        }

        // If we have added at least one production vertex,
        // our vertex is not the first one on the list
        if ( added ) continue;

        // If vertex not yet added
        if ( !v->in_event() ) {
            add_vertex(v);

            // Add all end vertices to the end of the list
            for (const auto& p: v->particles_out()) {
                GenVertexPtr v2 = p->end_vertex();
                if ( v2 && !v2->in_event()&& find(sorting.begin(), sorting.end(), v2) == sorting.end() ) {
                    sorting.emplace_back(v2);
                }
            }
        }

        sorting.pop_front();
    }

    // LL: Make sure root vertex has index zero and is not written out
    if ( m_rootvertex->id() != 0 ) {
        const int vx = -1 - m_rootvertex->id();
        const int rootid = m_rootvertex->id();
        if ( vx >= 0 && vx < (int) m_vertices.size() && m_vertices[vx] == m_rootvertex ) {
            auto next = m_vertices.erase(m_vertices.begin() + vx);
            std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
            for (auto & vt1: m_attributes) {
                std::vector< std::pair< int, std::shared_ptr<Attribute> > > changed_attributes;
                for ( const auto& vt2 : vt1.second ) {
                    if ( vt2.first <= rootid ) {
                        changed_attributes.emplace_back(vt2);
                    }
                }
                for ( const auto& val : changed_attributes ) {
                    vt1.second.erase(val.first);
                    vt1.second[val.first == rootid? 0: val.first + 1] = val.second;
                }
            }
            m_rootvertex->set_id(0);
            while ( next != m_vertices.end() ) {
                ++((*next++)->m_id);
            }
        } else {
            HEPMC3_WARNING("ReaderAsciiHepMC2: Suspicious looking rootvertex found. Will try to cope.")
        }
    }

    HEPMC3_DEBUG_CODE_BLOCK(
        HEPMC3_DEBUG(6, "GenEvent - particles sorted: "
                     << this->particles().size() << ", max deque size: "
                     << max_deque_size << ", iterations: " << sorting_loop_count)
    )
}


void GenEvent::reserve(const size_t& parts, const size_t& verts) {
    m_particles.reserve(parts);
    m_vertices.reserve(verts);
}


void GenEvent::set_units(Units::MomentumUnit new_momentum_unit, Units::LengthUnit new_length_unit) {
    if ( new_momentum_unit != m_momentum_unit ) {
        for ( GenParticlePtr& p: m_particles ) {
            Units::convert(p->m_data.momentum, m_momentum_unit, new_momentum_unit);
            Units::convert(p->m_data.mass, m_momentum_unit, new_momentum_unit);
        }

        m_momentum_unit = new_momentum_unit;
    }

    if ( new_length_unit != m_length_unit ) {
        for (GenVertexPtr& v: m_vertices) {
            FourVector &fv = v->m_data.position;
            if ( !fv.is_zero() ) Units::convert( fv, m_length_unit, new_length_unit );
        }

        m_length_unit = new_length_unit;
    }
}


const FourVector& GenEvent::event_pos() const {
    return m_rootvertex->data().position;
}

std::vector<ConstGenParticlePtr> GenEvent::beams(const int status) const {
    if (!status) return std::const_pointer_cast<const GenVertex>(m_rootvertex)->particles_out();
    std::vector<ConstGenParticlePtr> ret;
    for (auto p: m_rootvertex->particles_out()) if (p->status() == status) ret.emplace_back(p);
    return ret;
}

std::vector<ConstGenParticlePtr> GenEvent::beams() const {
    return std::const_pointer_cast<const GenVertex>(m_rootvertex)->particles_out();
}


const std::vector<GenParticlePtr> & GenEvent::beams() {
    return m_rootvertex->particles_out();
}

void GenEvent::shift_position_by(const FourVector & delta) {
    m_rootvertex->set_position(event_pos() + delta);

    // Offset all vertices
    for ( GenVertexPtr& v: m_vertices ) {
        if ( v->has_set_position() ) {
            v->set_position(v->position() + delta);
        }
    }
}

bool GenEvent::rotate(const FourVector&  delta)
{
    for ( auto& p: m_particles)
    {
        const FourVector& mom = p->momentum();
        long double tempX = mom.x();
        long double tempY = mom.y();
        long double tempZ = mom.z();

        long double cosa = std::cos(delta.x());
        long double sina = std::sin(delta.x());

        long double tempY_ = cosa*tempY+sina*tempZ;
        long double tempZ_ = -sina*tempY+cosa*tempZ;
        tempY = tempY_;
        tempZ = tempZ_;


        long double cosb = std::cos(delta.y());
        long double sinb = std::sin(delta.y());

        long double tempX_ = cosb*tempX-sinb*tempZ;
        tempZ_ = sinb*tempX+cosb*tempZ;
        tempX = tempX_;
        tempZ = tempZ_;

        long double cosg = std::cos(delta.z());
        long double sing = std::sin(delta.z());

        tempX_ = cosg*tempX+sing*tempY;
        tempY_ = -sing*tempX+cosg*tempY;
        tempX = tempX_;
        tempY = tempY_;

        FourVector temp(tempX, tempY, tempZ, mom.e());
        p->set_momentum(temp);
    }
    for (auto& v: m_vertices)
    {
        const FourVector& pos = v->position();
        long double tempX = pos.x();
        long double tempY = pos.y();
        long double tempZ = pos.z();

        long double cosa = std::cos(delta.x());
        long double sina = std::sin(delta.x());

        long double tempY_ = cosa*tempY+sina*tempZ;
        long double tempZ_ = -sina*tempY+cosa*tempZ;
        tempY = tempY_;
        tempZ = tempZ_;


        long double cosb = std::cos(delta.y());
        long double sinb = std::sin(delta.y());

        long double tempX_ = cosb*tempX-sinb*tempZ;
        tempZ_ = sinb*tempX+cosb*tempZ;
        tempX = tempX_;
        tempZ = tempZ_;

        long double cosg = std::cos(delta.z());
        long double sing = std::sin(delta.z());

        tempX_ = cosg*tempX+sing*tempY;
        tempY_ = -sing*tempX+cosg*tempY;
        tempX = tempX_;
        tempY = tempY_;

        FourVector temp(tempX, tempY, tempZ, pos.t());
        v->set_position(temp);
    }


    return true;
}

bool GenEvent::reflect(const int axis)
{
    if ( axis > 3 || axis < 0 )
    {
        HEPMC3_WARNING("GenEvent::reflect: wrong axis")
        return false;
    }
    switch (axis)
    {
    case 0:
        for ( auto& p: m_particles) { FourVector temp = p->momentum(); temp.setX(-p->momentum().x()); p->set_momentum(temp);}
        for ( auto& v: m_vertices)  { FourVector temp = v->position(); temp.setX(-v->position().x()); v->set_position(temp);}
        break;
    case 1:
        for ( auto& p: m_particles) { FourVector temp = p->momentum(); temp.setY(-p->momentum().y()); p->set_momentum(temp);}
        for ( auto& v: m_vertices)  { FourVector temp = v->position(); temp.setY(-v->position().y()); v->set_position(temp);}
        break;
    case 2:
        for ( auto& p: m_particles) { FourVector temp = p->momentum(); temp.setZ(-p->momentum().z()); p->set_momentum(temp);}
        for ( auto& v: m_vertices)  { FourVector temp = v->position(); temp.setZ(-v->position().z()); v->set_position(temp);}
        break;
    case 3:
        for ( auto& p: m_particles) { FourVector temp = p->momentum(); temp.setT(-p->momentum().e()); p->set_momentum(temp);}
        for ( auto& v: m_vertices)  { FourVector temp = v->position(); temp.setT(-v->position().t()); v->set_position(temp);}
        break;
    default:
        return false;
    }

    return true;
}

bool GenEvent::boost(const FourVector&  delta)
{
    double deltalength2d = delta.length2();
    if (deltalength2d > 1.0)
    {
        HEPMC3_WARNING("GenEvent::boost: wrong large boost vector. Will leave event as is.")
        return false;
    }
    if (std::abs(deltalength2d-1.0) < std::numeric_limits<double>::epsilon())
    {
        HEPMC3_WARNING("GenEvent::boost: too large gamma. Will leave event as is.")
        return false;
    }
    if (std::abs(deltalength2d) < std::numeric_limits<double>::epsilon())
    {
        HEPMC3_WARNING("GenEvent::boost: wrong small boost vector. Will leave event as is.")
        return true;
    }
    long double deltaX = delta.x();
    long double deltaY = delta.y();
    long double deltaZ = delta.z();
    long double deltalength2 = deltaX*deltaX+deltaY*deltaY+deltaZ*deltaZ;
    long double deltalength = std::sqrt(deltalength2);
    long double gamma = 1.0/std::sqrt(1.0-deltalength2);

    for ( auto& p: m_particles)
    {
        const FourVector& mom = p->momentum();

        long double tempX = mom.x();
        long double tempY = mom.y();
        long double tempZ = mom.z();
        long double tempE = mom.e();
        long double nr = (deltaX*tempX+deltaY*tempY+deltaZ*tempZ)/deltalength;

        tempX+=(deltaX*((gamma-1)*nr/deltalength)-deltaX*(tempE*gamma));
        tempY+=(deltaY*((gamma-1)*nr/deltalength)-deltaY*(tempE*gamma));
        tempZ+=(deltaZ*((gamma-1)*nr/deltalength)-deltaZ*(tempE*gamma));
        tempE = gamma*(tempE-deltalength*nr);
        FourVector temp(tempX, tempY, tempZ, tempE);
        p->set_momentum(temp);
    }

    return true;
}

void GenEvent::clear() {
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    m_event_number = 0;
    m_rootvertex = std::make_shared<GenVertex>();
    m_weights.clear();
    m_attributes.clear();
    m_particles.clear();
    m_vertices.clear();
}

void GenEvent::remove_attribute(const std::string &name,  const int& id) {
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    auto i1 = m_attributes.find(name);
    if ( i1 == m_attributes.end() ) return;

    auto i2 = i1->second.find(id);
    if ( i2 == i1->second.end() ) return;

    i1->second.erase(i2);
}

std::vector<std::string> GenEvent::attribute_names(const int& id) const {
    std::vector<std::string> results;

    for (const att_key_t& vt1: m_attributes) {
        if ( vt1.second.count(id) == 1 ) {
            results.emplace_back(vt1.first);
        }
    }

    return results;
}

void GenEvent::write_data(GenEventData& data) const {
    // Reserve memory for containers
    data.particles.reserve(this->particles().size());
    data.vertices.reserve(this->vertices().size());
    data.links1.reserve(this->particles().size()*2);
    data.links2.reserve(this->particles().size()*2);
    data.attribute_id.reserve(m_attributes.size());
    data.attribute_name.reserve(m_attributes.size());
    data.attribute_string.reserve(m_attributes.size());

    // Fill event data
    data.event_number  = this->event_number();
    data.momentum_unit = this->momentum_unit();
    data.length_unit   = this->length_unit();
    data.event_pos     = this->event_pos();

    // Fill containers
    data.weights = this->weights();

    for (const ConstGenParticlePtr& p: this->particles()) {
        data.particles.emplace_back(p->data());
    }

    for (const ConstGenVertexPtr& v: this->vertices()) {
        data.vertices.emplace_back(v->data());
        int v_id = v->id();

        for (const ConstGenParticlePtr& p: v->particles_in()) {
            data.links1.emplace_back(p->id());
            data.links2.emplace_back(v_id);
        }

        for (const ConstGenParticlePtr& p: v->particles_out()) {
            data.links1.emplace_back(v_id);
            data.links2.emplace_back(p->id());
        }
    }

    for (const att_key_t& vt1: this->attributes()) {
        for (const att_val_t& vt2: vt1.second) {
            std::string st;

            bool status = vt2.second->to_string(st);

            if ( !status ) {
                HEPMC3_WARNING("GenEvent::write_data: problem serializing attribute: " << vt1.first)
            }
            else {
                data.attribute_id.emplace_back(vt2.first);
                data.attribute_name.emplace_back(vt1.first);
                data.attribute_string.emplace_back(st);
            }
        }
    }
}


void GenEvent::read_data(const GenEventData &data) {
    this->clear();
    this->set_event_number(data.event_number);
    //Note: set_units checks the current unit of event, i.e. applicable only for fully constructed event.
    m_momentum_unit = data.momentum_unit;
    m_length_unit = data.length_unit;
    this->shift_position_to(data.event_pos);

    // Fill weights
    this->weights() = data.weights;
    m_particles.reserve(data.particles.size());
    m_vertices.reserve(data.vertices.size());

    // Fill particle information
    for ( const GenParticleData &pd: data.particles ) {
        m_particles.emplace_back(std::make_shared<GenParticle>(pd));
        m_particles.back()->m_event = this;
        m_particles.back()->m_id    = m_particles.size();
    }

    // Fill vertex information
    for ( const GenVertexData &vd: data.vertices ) {
        m_vertices.emplace_back(std::make_shared<GenVertex>(vd));
        m_vertices.back()->m_event = this;
        m_vertices.back()->m_id    = -(int)m_vertices.size();
    }

    // Restore links
    for (unsigned int i = 0; i < data.links1.size(); ++i) {
        const int id1 = data.links1[i];
        const int id2 = data.links2[i];
        /* @note:
        The  meaningfull combinations for (id1,id2) are:
        (+-)  --  particle has end vertex
        (-+)  --  particle  has production vertex
        */
        if ((id1 < 0 && id2 <0) || (id1 > 0 && id2 > 0))   { HEPMC3_WARNING("GenEvent::read_data: wrong link: " << id1 << " " << id2); continue;}

        if ( id1 > 0 ) { m_vertices[ (-id2)-1 ]->add_particle_in ( m_particles[ id1-1 ] ); continue; }
        if ( id1 < 0 ) { m_vertices[ (-id1)-1 ]->add_particle_out( m_particles[ id2-1 ] );   continue; }
    }
    for (auto& p:  m_particles) if (!p->production_vertex()) m_rootvertex->add_particle_out(p);

    // Read attributes
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    for (unsigned int i = 0; i < data.attribute_id.size(); ++i) {
        ///Disallow empty strings
        const std::string name = data.attribute_name[i];
        if (name.length() == 0) continue;
        const int id = data.attribute_id[i];
        if (m_attributes.count(name) == 0) m_attributes[name] = std::map<int, std::shared_ptr<Attribute> >();
        auto att = std::make_shared<StringAttribute>(data.attribute_string[i]);
        att->m_event = this;
        if ( id > 0 && id <= int(m_particles.size()) ) {
            att->m_particle = m_particles[id - 1];
        }
        if ( id < 0 && -id <= int(m_vertices.size()) ) {
            att->m_vertex = m_vertices[-id - 1];
        }
        m_attributes[name][id] = att;
    }
}


//
// Deprecated functions
//

void GenEvent::add_particle(GenParticle *p) {
    add_particle(GenParticlePtr(p));
}


void GenEvent::add_vertex(GenVertex *v) {
    add_vertex(GenVertexPtr(v));
}


void GenEvent::set_beam_particles(GenParticlePtr p1, GenParticlePtr p2) {
    m_rootvertex->add_particle_out(p1);
    m_rootvertex->add_particle_out(p2);
}

void GenEvent::add_beam_particle(GenParticlePtr p1) {
    if (!p1)
    {
        HEPMC3_WARNING("Attempting to add an empty particle as beam particle. Ignored.")
        return;
    }
    if (p1->in_event() && p1->parent_event() != this)
    {
        HEPMC3_WARNING("Attempting to add particle from another event. Ignored.")
        return;
    }
    if (p1->production_vertex())  p1->production_vertex()->remove_particle_out(p1);
    //Particle w/o production vertex is added to root vertex.
    add_particle(p1);
    p1->set_status(4);
}


std::string GenEvent::attribute_as_string(const std::string &name, const int& id) const {
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    auto i1 = m_attributes.find(name);
    if ( i1 == m_attributes.end() ) {
        if ( id == 0 && run_info() ) {
            return run_info()->attribute_as_string(name);
        }
        return {};
    }

    auto i2 = i1->second.find(id);
    if (i2 == i1->second.end() ) return {};

    if ( !i2->second ) return {};

    std::string ret;
    i2->second->to_string(ret);

    return ret;
}

void GenEvent::add_attribute(const std::string &name, const std::shared_ptr<Attribute> &att, const int& id ) {
    ///Disallow empty strings
    if (name.length() == 0) return;
    if (!att)  return;
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    if (m_attributes.count(name) == 0) m_attributes[name] = std::map<int, std::shared_ptr<Attribute> >();
    m_attributes[name][id] = att;
    att->m_event = this;
    if ( id > 0 && id <= int(particles().size()) ) {
        att->m_particle = particles()[id - 1];
    }
    if ( id < 0 && -id <= int(vertices().size()) ) {
        att->m_vertex = vertices()[-id - 1];
    }
}


void GenEvent::add_attributes(const std::vector<std::string> &names, const std::vector<std::shared_ptr<Attribute> > &atts, const std::vector<int>& ids) {
    size_t N = names.size();
    if ( N == 0 ) return;
    if (N != atts.size()) return;
    if (N != ids.size()) return;

    std::vector<std::string> unames = names;
    vector<std::string>::iterator ip;
    ip = std::unique(unames.begin(), unames.end());
    unames.resize(std::distance(unames.begin(), ip));
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    for (const auto& name: unames) {
        if (m_attributes.count(name) == 0) m_attributes[name] = std::map<int, std::shared_ptr<Attribute> >();
    }
    const int particles_size = int(m_particles.size());
    const int vertices_size = int(m_vertices.size());
    for (size_t i = 0; i < N; i++) {
        ///Disallow empty strings
        if (names.at(i).length() == 0) continue;
        if (!atts[i])  continue;
        m_attributes[names.at(i)][ids.at(i)] = atts[i];
        atts[i]->m_event = this;
        if ( ids.at(i) > 0 && ids.at(i) <= particles_size )
        { atts[i]->m_particle = m_particles[ids.at(i) - 1]; }
        else {
            if ( ids.at(i) < 0 && -ids.at(i) <= vertices_size ) {
                atts[i]->m_vertex = m_vertices[-ids.at(i) - 1];
            }
        }
    }
}

void GenEvent::add_attributes(const std::string& name, const std::vector<std::shared_ptr<Attribute> > &atts, const std::vector<int>& ids) {
    if (name.length() == 0) return;
    size_t N = ids.size();
    if(!N) return;
    if ( N != atts.size()) return;

    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    if (m_attributes.count(name) == 0) m_attributes[name] = std::map<int, std::shared_ptr<Attribute> >();
    auto& tmap = m_attributes[name];
    const int particles_size = int(m_particles.size());
    const int vertices_size = int(m_vertices.size());
    for (size_t i = 0; i < N; i++) {
        ///Disallow empty strings
        if (!atts[i])  continue;
        tmap[ids.at(i)] = atts[i];
        atts[i]->m_event = this;
        if ( ids.at(i) > 0 && ids.at(i) <= particles_size )
        { atts[i]->m_particle = m_particles[ids.at(i) - 1]; }
        else {
            if ( ids.at(i) < 0 && -ids.at(i) <= vertices_size ) {
                atts[i]->m_vertex = m_vertices[-ids.at(i) - 1];
            }
        }
    }
}
void GenEvent::add_attributes(const std::string& name, const std::vector<std::pair<int, std::shared_ptr<Attribute> > > &atts) {
    if (name.length() == 0) return;
    if (atts.empty()) return;
    std::lock_guard<std::recursive_mutex> lock(m_lock_attributes);
    if (m_attributes.count(name) == 0) m_attributes[name] = std::map<int, std::shared_ptr<Attribute> >();
    auto& tmap = m_attributes[name];
    const int particles_size = int(m_particles.size());
    const int vertices_size = int(m_vertices.size());
    for (const auto& att: atts) {
        ///Disallow empty strings
        if (!att.second)  continue;
        tmap.insert(att);
        att.second->m_event = this;
        if ( att.first > 0 && att.first <= particles_size )
        { att.second->m_particle = m_particles[att.first - 1]; }
        else {
            if ( att.first < 0 && -att.first <= vertices_size ) {
                att.second->m_vertex = m_vertices[-att.first - 1];
            }
        }
    }
}

} // namespace HepMC3
