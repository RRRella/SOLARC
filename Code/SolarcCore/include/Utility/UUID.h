#pragma once
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_hash.hpp>
#include <boost/uuid/uuid_io.hpp>

using UUID = boost::uuids::uuid;

inline void GenerateUUID(UUID& out) {
    static thread_local boost::uuids::random_generator generator;
    out = generator();
}
