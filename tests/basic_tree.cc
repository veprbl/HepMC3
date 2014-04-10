/**
 * @brief A throwaway test of installation
 *
 * Based on HepMC2/examples/example_BuildEventFromScratch.cc
 */
#include "HepMC3/GenEvent.h"
#include "HepMC3/GenVertex.h"
#include "HepMC3/GenParticle.h"
#include "HepMC3/Search/FindParticles.h"

#include <iostream>

#include <boost/foreach.hpp>
using namespace HepMC3;

int main() {
    //
    // In this example we will place the following event into HepMC "by hand"
    //
    //     name status pdg_id  parent Px       Py    Pz       Energy      Mass
    //  1  !p+!    3   2212    0,0    0.000    0.000 7000.000 7000.000    0.938
    //  2  !p+!    3   2212    0,0    0.000    0.000-7000.000 7000.000    0.938
    //=========================================================================
    //  3  !d!     3      1    1,1    0.750   -1.569   32.191   32.238    0.000
    //  4  !u~!    3     -2    2,2   -3.047  -19.000  -54.629   57.920    0.000
    //  5  !W-!    3    -24    1,2    1.517   -20.68  -20.605   85.925   80.799
    //  6  !gamma! 1     22    1,2   -3.813    0.113   -1.833    4.233    0.000
    //  7  !d!     1      1    5,5   -2.445   28.816    6.082   29.552    0.010
    //  8  !u~!    1     -2    5,5    3.962  -49.498  -26.687   56.373    0.006

    // now we build the graph, which will looks like
    //                       p7                         #
    // p1                   /                           #
    //   \v1__p3      p5---v4                           #
    //         \_v3_/       \                           #
    //         /    \        p8                         #
    //    v2__p4     \                                  #
    //   /            p6                                #
    // p2                                               #
    //                                                  #
    GenEvent evt;

    GenParticle &p1 = evt.new_particle();
    p1.set_momentum( FourVector(0,0,7000,700) );
    p1.set_pdg_id(2212);
    p1.set_status(3);

    GenParticle &p2 = evt.new_particle();
    p2.set_momentum( FourVector(0,0,-7000,700) );
    p2.set_pdg_id(2212);
    p2.set_status(3);

    GenParticle &p3 = evt.new_particle();
    p3.set_momentum( FourVector(.750,-1.569,32.191,32.238) );
    p3.set_pdg_id(1);
    p3.set_status(3);

    GenParticle &p4 = evt.new_particle();
    p4.set_momentum( FourVector(-3.047,-19.,-54.629,57.920) );
    p4.set_pdg_id(-2);
    p4.set_status(3);

    GenVertex &v1 = evt.new_vertex();
    v1.add_particle_in ( p1 );
    v1.add_particle_out( p3 );

    GenVertex &v2 = evt.new_vertex();
    v2.add_particle_in ( p2 );
    v2.add_particle_out( p4 );

    GenVertex &v3 = evt.new_vertex();
    v3.add_particle_in( p3 );
    v3.add_particle_in( p4 );

    GenParticle &p5 = evt.new_particle();
    p5.set_momentum( FourVector(-3.813,0.113,-1.833,4.233 ) );
    p5.set_pdg_id(22);
    p5.set_status(1);

    GenParticle &p6 = evt.new_particle();
    p6.set_momentum( FourVector(1.517,-20.68,-20.605,85.925) );
    p6.set_pdg_id(-24);
    p6.set_status(3);

    v3.add_particle_out( p5 );
    v3.add_particle_out( p6 );

    GenVertex &v4 = evt.new_vertex();
    v4.add_particle_in ( p6 );

    GenParticle &p7 = evt.new_particle();
    p7.set_momentum( FourVector(-2.445,28.816,6.082,29.552) );
    p7.set_pdg_id(1);
    p7.set_status(1);

    GenParticle &p8 = evt.new_particle();
    p8.set_momentum( FourVector(3.962,-49.498,-26.687,56.373) );
    p8.set_pdg_id(-2);
    p8.set_status(1);

    v4.add_particle_out( p7 );
    v4.add_particle_out( p8 );

    evt.print();

/*
    // versioning test
    evt->create_new_version("Second tool");

    evt->delete_particle( p7 );

    GenParticle* p9 = new GenParticle( FourVector(1., 2., 3., 4.), 1,1 );
    evt->add_particle( p9 );
    v4->add_particle_out( p9 );

    evt->create_new_version("Third tool");

    evt->delete_particle(p4);

    // printout
    evt->set_current_version(0);
    evt->print();
    evt->set_current_version(1);
    evt->print();
    evt->set_current_version(evt->last_version());
    evt->print();
*/

    std::cout<<std::endl<<"Find all stable particles: "<<std::endl;
    FindParticles search(evt, FIND_ALL, STATUS == 1 && STATUS_SUBCODE == 0 );

    BOOST_FOREACH( const GenParticle *p, search.results() ) {
        p->print();
    }

    std::cout<<std::endl<<"Find all ancestors of particle with barcode "<<p5.barcode()<<": "<<std::endl;
    FindParticles search2(evt, p5, FIND_ALL_ANCESTORS );

    BOOST_FOREACH( const GenParticle *p, search2.results() ) {
        p->print();
    }

    std::cout<<std::endl<<"Find stable descendants of particle with barcode "<<p4.barcode()<<": "<<std::endl;
    std::cout<<"(just for test, we check both for status == 1 and no end vertex)"<<std::endl;
    FindParticles search3(evt, p4, FIND_ALL_DESCENDANTS, STATUS == 1 && !HAS_END_VERTEX );

    BOOST_FOREACH( const GenParticle *p, search3.results() ) {
        p->print();
    }

    std::cout<<std::endl<<"Narrow down search results to quarks: "<<std::endl;
    search3.narrow_down( PDG_ID >= -6 && PDG_ID <= 6 );

    BOOST_FOREACH( const GenParticle *p, search3.results() ) {
        p->print();
    }

    return 0;
}
