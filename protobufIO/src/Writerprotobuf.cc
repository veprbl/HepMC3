// -*- C++ -*-
//
// This file is part of HepMC
// Copyright (C) 2014-2022 The HepMC collaboration (see AUTHORS for details)
//
/**
 *  @file Writerprotobuf.cc
 *  @brief Implementation of \b class Writerprotobuf
 *
 */
#include "HepMC3/Writerprotobuf.h"
#include "HepMC3/Data/GenEventData.h"
#include "HepMC3/Data/GenRunInfoData.h"
#include "HepMC3/Version.h"

// protobuf header files
#include "HepMC3/HepMC3.pb.h"

namespace HepMC3 {
HEPMC3_DECLARE_WRITER_FILE(Writerprotobuf);
HEPMC3_DECLARE_WRITER_STREAM(Writerprotobuf);

template <typename T>
size_t write_message(std::ostream *out_stream, T &msg,
                     HepMC3_pb::MessageDigest::MessageType type) {

  std::string msg_str;
  msg.SerializeToString(&msg_str);

  HepMC3_pb::MessageDigest md;
  md.set_bytes(msg_str.size());
  md.set_message_type(type);

  std::string md_str;
  md.SerializeToString(&md_str);

  if (md_str.size() != 10) {
    HEPMC3_ERROR("When writing protobuf message, the message digest was not "
                 "the expected length (10 bytes), but was instead "
                 << md_str.size() << " bytes.");
  }

  // std::cout << "[MessageDigest]: size " << md_str.size()
  //           << " bytes\n>>>>>>>>>>>>>>>>>>\n"
  //           << md.DebugString() << "<<<<<<<<<<<<<<<<<<" << std::endl;
  // std::cout << "[Message]: size " << msg_str.size()
  //           << " bytes\n>>>>>>>>>>>>>>>>>>\n"
  //           << msg.DebugString() << "<<<<<<<<<<<<<<<<<<" << std::endl;

  (*out_stream) << md_str;

  (*out_stream) << msg_str;
  return md_str.size() + msg_str.size();
}

Writerprotobuf::Writerprotobuf(const std::string &filename,
                               std::shared_ptr<GenRunInfo> run)
    : out_file(nullptr), number_of_events_written(0), event_bytes_written(0) {

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (!run) {
    run = std::shared_ptr<GenRunInfo>(new GenRunInfo());
  }
  set_run_info(run);

  // open file
  out_file = std::unique_ptr<ofstream>(
      new ofstream(filename, ios::out | ios::trunc | ios::binary));

  // check that it is open
  if (!out_file->is_open()) {
    HEPMC3_ERROR("Writerprotobuf: problem opening file: " << filename)
    return;
  }

  out_stream = out_file.get();
  start_file();
}

Writerprotobuf::Writerprotobuf(std::ostream &stream,
                               std::shared_ptr<GenRunInfo> run)
    : out_file(nullptr), number_of_events_written(0), event_bytes_written(0) {

  if (!stream.good()) {
    HEPMC3_ERROR(
        "Cannot initialize Writerprotobuf on ostream which is not good().");
    return;
  }

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  if (!run) {
    run = std::make_shared<GenRunInfo>();
  }
  set_run_info(run);

  out_stream = &stream;
  start_file();
}

void Writerprotobuf::start_file() {
  (*out_stream) << std::string(
      "HepMC3::Protobuf"); // The first 16 bytes of a HepMC protobuf file

  HepMC3_pb::Header hdr;
  (*hdr.mutable_version_str()) = HepMC3::version();
  hdr.set_version_maj((HEPMC3_VERSION_CODE / 1000000) % 1000);
  hdr.set_version_min((HEPMC3_VERSION_CODE / 1000) % 1000);
  hdr.set_version_patch(HEPMC3_VERSION_CODE % 1000);
  write_message(out_stream, hdr, HepMC3_pb::MessageDigest::Header);

  write_run_info();
}

void Writerprotobuf::write_event(const GenEvent &evt) {

  GenEventData data;
  evt.write_data(data);

  HepMC3_pb::GenEventData ged_pb;
  ged_pb.set_event_number(data.event_number);

  switch (data.momentum_unit) {
  case HepMC3::Units::MEV: {
    ged_pb.set_momentum_unit(HepMC3_pb::GenEventData::MEV);
    break;
  }
  case HepMC3::Units::GEV: {
    ged_pb.set_momentum_unit(HepMC3_pb::GenEventData::GEV);
    break;
  }
  default: {
    HEPMC3_ERROR("Unknown momentum unit: " << data.momentum_unit);
    abort();
  }
  }

  switch (data.length_unit) {
  case HepMC3::Units::MM: {
    ged_pb.set_length_unit(HepMC3_pb::GenEventData::MM);
    break;
  }
  case HepMC3::Units::CM: {
    ged_pb.set_length_unit(HepMC3_pb::GenEventData::CM);
    break;
  }
  default: {
    HEPMC3_ERROR("Unknown length unit: " << data.length_unit);
    abort();
  }
  }

  for (auto const &pdata : data.particles) {
    auto particle_pb = ged_pb.add_particles();
    particle_pb->set_pid(pdata.pid);
    particle_pb->set_status(pdata.status);
    particle_pb->set_is_mass_set(pdata.is_mass_set);
    particle_pb->set_mass(pdata.mass);

    particle_pb->mutable_momentum()->set_m_v1(pdata.momentum.x());
    particle_pb->mutable_momentum()->set_m_v2(pdata.momentum.y());
    particle_pb->mutable_momentum()->set_m_v3(pdata.momentum.z());
    particle_pb->mutable_momentum()->set_m_v4(pdata.momentum.t());
  }

  for (auto const &vdata : data.vertices) {
    auto vertex_pb = ged_pb.add_vertices();
    vertex_pb->set_status(vdata.status);

    vertex_pb->mutable_position()->set_m_v1(vdata.position.x());
    vertex_pb->mutable_position()->set_m_v2(vdata.position.y());
    vertex_pb->mutable_position()->set_m_v3(vdata.position.z());
    vertex_pb->mutable_position()->set_m_v4(vdata.position.t());
  }

  for (auto const &s : data.weights) {
    ged_pb.add_weights(s);
  }

  for (auto const &s : data.links1) {
    ged_pb.add_links1(s);
  }
  for (auto const &s : data.links2) {
    ged_pb.add_links2(s);
  }

  ged_pb.mutable_event_pos()->set_m_v1(data.event_pos.x());
  ged_pb.mutable_event_pos()->set_m_v2(data.event_pos.y());
  ged_pb.mutable_event_pos()->set_m_v3(data.event_pos.z());
  ged_pb.mutable_event_pos()->set_m_v4(data.event_pos.t());

  for (auto const &s : data.attribute_id) {
    ged_pb.add_attribute_id(s);
  }
  for (auto const &s : data.attribute_name) {
    ged_pb.add_attribute_name(s);
  }
  for (auto const &s : data.attribute_string) {
    ged_pb.add_attribute_string(s);
  }

  event_bytes_written +=
      write_message(out_stream, ged_pb, HepMC3_pb::MessageDigest::Event);
  number_of_events_written++;
}

void Writerprotobuf::write_run_info() {

  GenRunInfoData data;
  run_info()->write_data(data);

  HepMC3_pb::GenRunInfoData GenRunInfo_pb;

  for (auto const &s : data.weight_names) {
    GenRunInfo_pb.add_weight_names(s);
  }

  for (auto const &s : data.tool_name) {
    GenRunInfo_pb.add_tool_name(s);
  }
  for (auto const &s : data.tool_version) {
    GenRunInfo_pb.add_tool_version(s);
  }
  for (auto const &s : data.tool_description) {
    GenRunInfo_pb.add_tool_description(s);
  }

  for (auto const &s : data.attribute_name) {
    GenRunInfo_pb.add_attribute_name(s);
  }
  for (auto const &s : data.attribute_string) {
    GenRunInfo_pb.add_attribute_string(s);
  }

  write_message(out_stream, GenRunInfo_pb, HepMC3_pb::MessageDigest::RunInfo);
}

void Writerprotobuf::close() {
  if (failed()) {
    return;
  }

  if (!number_of_events_written) {
    HEPMC3_ERROR(
        "No events were written, the output file will not be parseable.");
  }

  HepMC3_pb::Footer ftr;
  ftr.set_nevents(number_of_events_written);
  ftr.set_event_bytes_written(event_bytes_written);
  write_message(out_stream, ftr, HepMC3_pb::MessageDigest::Footer);

  if (out_file) {
    out_file->close();
    out_file.reset();
  }
  out_stream = nullptr;
}

bool Writerprotobuf::failed() {
  if (out_file) {
    return !out_file || !out_file->is_open() || !out_file->good();
  }
  return !out_stream || !out_stream->good();
}

} // namespace HepMC3