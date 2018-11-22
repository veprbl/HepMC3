// -*- C++ -*-
//
// This file is part of HepMC
// Copyright (C) 2014-2018 The HepMC collaboration (see AUTHORS for details)
//
///
/// @file Relatives.h
/// @brief Defines helper classes to extract relatives of an input GenParticle or GenVertex
///
#include "HepMC/GenParticle.fh"
#include "HepMC/GenVertex.fh"

namespace HepMC{

  /** @brief  Define a common interface that all Relatives objects will satisfy
   *         Relatives provides an operator to get the relatives of a range of different
   *         GenObject types
   */
  class Relatives{
    
  public:
    
    virtual std::vector<GenParticlePtr> operator()(GenParticlePtr input) const = 0;
    virtual std::vector<ConstGenParticlePtr> operator()(ConstGenParticlePtr input) const = 0;
    virtual std::vector<GenParticlePtr> operator()(GenVertexPtr input) const = 0;
    virtual std::vector<ConstGenParticlePtr> operator()(ConstGenVertexPtr input) const = 0;
    
  };
  
  /** @brief wrap a templated class that implements Relatives
   *  Since we need to template the functionality on the input
   *  type (GenParticlePtr, ConstGenVertexPtr etc.) we must wrap a
   *  class that has a templated operator in this that provides the
   *  Relatives interface and calls through to the underlying template
   *  method.
   */
  template<typename Relative_type>
  class RelativesInterface : public Relatives{
    
  public:
    
    //RelativesInterface(Relative_type relatives): _internal(relatives){}
    
    constexpr RelativesInterface(){}
    
    GenParticles_type<GenParticlePtr> operator()(GenParticlePtr input) const override {return _internal(input);}
    GenParticles_type<ConstGenParticlePtr> operator()(ConstGenParticlePtr input) const override {return _internal(input);}
    GenParticles_type<GenVertexPtr> operator()(GenVertexPtr input) const override {return _internal(input);}
    GenParticles_type<ConstGenVertexPtr> operator()(ConstGenVertexPtr input) const override {return _internal(input);}
    
  private:
    
    Relative_type _internal;
    
  };
  
  /** @brief Provides operator to find the parent particles of a Vertex or Particle
   *
   * Note you would usually not instantiate this directly, but wrap it in a RelativesInterface
   */
  class _parents{
    
  public:
    
    template<typename GenObject_type, typename dummy>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const;
    
    template<typename GenObject_type, typename std::enable_if<std::is_same<GenVertex, typename std::remove_const<typename GenObject_type::element_type>::type>::value, int*>::type = nullptr>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const {return input->particles_in();}

    template<typename GenObject_type, typename std::enable_if<std::is_same<GenParticle, typename std::remove_const<typename GenObject_type::element_type>::type>::value, int*>::type = nullptr>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const {return (*this)(vertex(input));}
    
    template<typename GenObject_type>
    GenVertex_type<GenObject_type> vertex(GenObject_type input) const {return input->production_vertex();}
    
  };
  
  /** @brief Provides operator to find the child particles of a Vertex or Particle
   *
   * Note you would usually not instantiate this directly, but wrap it in a RelativesInterface
   */
  class _children{
    
  public:
    
    template<typename GenObject_type, typename dummy>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const;
    
    template<typename GenObject_type, typename std::enable_if<std::is_same<GenVertex, typename std::remove_const<typename GenObject_type::element_type>::type>::value, int*>::type = nullptr>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const {return input->particles_out();}
    
    template<typename GenObject_type, typename std::enable_if<std::is_same<GenParticle, typename std::remove_const<typename GenObject_type::element_type>::type>::value, int*>::type = nullptr>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const {return (*this)(vertex(input));}
    
    template<typename GenObject_type>
    GenVertex_type<GenObject_type> vertex(GenObject_type input) const {return input->end_vertex();}
    
  };
  
  
  template<typename Relation_type>
  class Recursive{
    
  public:
    
    template<typename GenObject_type>
    GenParticles_type<GenObject_type> operator()(GenObject_type input) const {
      for(auto obj: m_checkedObjects){
        delete obj;
      }
      m_checkedObjects.clear();
      return _recursive(input);
    }
    
  private:
    
    template<typename GenObject_type, typename dummy>
    GenParticles_type<GenObject_type> _recursive(GenObject_type input) const ;
    
    template<typename GenObject_type, typename std::enable_if<std::is_same<GenVertex, typename std::remove_const<typename GenObject_type::element_type>::type>::value, int*>::type = nullptr>
    GenParticles_type<GenObject_type> _recursive(GenObject_type input) const {
      
      GenParticles_type <GenObject_type> results;
      for(auto v: m_checkedObjects){
        if(v->id() == input->id()) return results;
      }
      
      m_checkedObjects.emplace_back(new idInterface<GenObject_type>(input));
      
      for(auto p: m_applyRelation(input)){
        results.emplace_back(p);
        GenParticles_type <GenObject_type> tmp = _recursive(p);
        results.insert(results.end(),
                       std::make_move_iterator(tmp.begin()),
                       std::make_move_iterator(tmp.end()));
      }
     
      return results;
    }
    
    template<typename GenObject_type, typename std::enable_if<std::is_same<GenParticle, typename std::remove_const<typename GenObject_type::element_type>::type>::value, int*>::type = nullptr>
    GenParticles_type<GenObject_type> _recursive(GenObject_type input) const {
     return _recursive(m_applyRelation.vertex(input));
    }
      
    class hasId{
      
    public:
      virtual ~hasId(){}
      virtual int id() const = 0;
    };
    
    template<typename ID_type>
    class idInterface : public hasId{
      
    public:
      constexpr idInterface(ID_type genObject): m_object(genObject){}
      int id() const {return m_object->id();}
      
    private:
      
      ID_type m_object;
      
    };
    
    Relation_type m_applyRelation;
    mutable std::vector<hasId*> m_checkedObjects;
    
  };
  
  /// alias of _parents wrapped in the Relatives interface
  using Parents  = RelativesInterface<_parents>;
  /// alias of _children wrapped in the Relatives interface
  using Children = RelativesInterface<_children>;
  /// Ancestors is an alias to Recursion applied to the _parents and wrapped in the Relatives interface
  using Ancestors = RelativesInterface<Recursive<_parents> >;
  /// Descendants is an alias to Recursion applied to the _children and wrapped in the Relatives interface
  using Descendants = RelativesInterface<Recursive<_children> >;

}

