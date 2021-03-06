#include <string>
#include <set>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <chrono>
#include <sstream>
#ifdef WITH_MPI
#include <mpi.h>
#endif

#include <hpx/config.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TCompactProtocol.h>

#include "thrift/gen-cpp/test_types.h"
#include "thrift/gen-cpp/test_constants.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include "protobuf/test.pb.h"
#include "capnproto/test.capnp.h"
#include "boost/record.hpp"
#include "msgpack/record.hpp"
#include "cereal/record.hpp"
#include "avro/record.hpp"
#include "hpx/record.hpp"
#include "hpx_zero_copy/record.hpp"
#include "hpx/version.hpp"
#include "mpi/record.hpp"
#include "yas/record.hpp"
#include "flatbuffers/test_generated.h"

#include "data.hpp"

enum class ThriftSerializationProto {
    Binary,
    Compact
};

void
thrift_serialization_test(size_t iterations, ThriftSerializationProto proto = ThriftSerializationProto::Binary)
{
    using apache::thrift::transport::TMemoryBuffer;
    using apache::thrift::protocol::TBinaryProtocol;
    using apache::thrift::protocol::TCompactProtocol;

    using namespace thrift_test;

    boost::shared_ptr<TMemoryBuffer> buffer1(new TMemoryBuffer());
    boost::shared_ptr<TMemoryBuffer> buffer2(new TMemoryBuffer());

    TBinaryProtocol binary_protocol1(buffer1);
    TBinaryProtocol binary_protocol2(buffer2);

    TCompactProtocol compact_protocol1(buffer1);
    TCompactProtocol compact_protocol2(buffer2);

    Record r1;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::string serialized;

    if (proto == ThriftSerializationProto::Binary) {
        r1.write(&binary_protocol1);
    } else if (proto == ThriftSerializationProto::Compact) {
        r1.write(&compact_protocol1);
    }

    serialized = buffer1->getBufferAsString();

    // check if we can deserialize back
    Record r2;

    buffer2->resetBuffer((uint8_t*)serialized.data(), serialized.length());

    if (proto == ThriftSerializationProto::Binary) {
        r2.read(&binary_protocol2);
    } else if (proto == ThriftSerializationProto::Compact) {
        r2.read(&compact_protocol2);
    }

    if (r1 != r2) {
        throw std::logic_error("thrift's case: deserialization failed");
    }

    std::string tag;

    if (proto == ThriftSerializationProto::Binary) {
        tag = "thrift-binary:";
    } else if (proto == ThriftSerializationProto::Compact) {
        tag = "thrift-compact:";
    }

    std::cout << tag << " version = " << VERSION << std::endl;
    std::cout << tag << " size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        buffer1->resetBuffer();

        if (proto == ThriftSerializationProto::Binary) {
            r1.write(&binary_protocol1);
        } else if (proto == ThriftSerializationProto::Compact) {
            r1.write(&compact_protocol1);
        }

        serialized = buffer1->getBufferAsString();
        buffer2->resetBuffer((uint8_t*)serialized.data(), serialized.length());

        if (proto == ThriftSerializationProto::Binary) {
            r2.read(&binary_protocol2);
        } else if (proto == ThriftSerializationProto::Compact) {
            r2.read(&compact_protocol2);
        }
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << tag << " time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
protobuf_serialization_test(size_t iterations)
{
    using namespace protobuf_test;

    Record r1;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.add_ids(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.add_strings(kStringValue);
    }

    std::string serialized;

    r1.SerializeToString(&serialized);

    // check if we can deserialize back
    Record r2;
    bool ok = r2.ParseFromString(serialized);
    if (!ok /*|| r2 != r1*/) {
        throw std::logic_error("protobuf's case: deserialization failed");
    }

    std::cout << "protobuf: version = " << GOOGLE_PROTOBUF_VERSION << std::endl;
    std::cout << "protobuf: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized.clear();
        r1.SerializeToString(&serialized);
        r2.ParseFromString(serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "protobuf: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
capnproto_serialization_test(size_t iterations)
{
    using namespace capnp_test;

    capnp::MallocMessageBuilder message;
    Record::Builder r1 = message.getRoot<Record>();

    auto ids = r1.initIds(kIntegers.size());
    for (size_t i = 0; i < kIntegers.size(); i++) {
        ids.set(i, kIntegers[i]);
    }

    auto strings = r1.initStrings(kStringsCount);
    for (size_t i = 0; i < kStringsCount; i++) {
        strings.set(i, kStringValue);
    }

    kj::ArrayPtr<const kj::ArrayPtr<const capnp::word>> serialized =
        message.getSegmentsForOutput();

    // check if we can deserialize back
    capnp::SegmentArrayMessageReader reader(serialized);
    Record::Reader r2 = reader.getRoot<Record>();
    if (r2.getIds().size() != kIntegers.size()) {
        throw std::logic_error("capnproto's case: deserialization failed");
    }

    size_t size = 0;
    for (auto segment: serialized) {
      size += segment.asBytes().size();
    }

    std::cout << "capnproto: version = " << CAPNP_VERSION << std::endl;
    std::cout << "capnproto: size = " << size << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized = message.getSegmentsForOutput();
        capnp::SegmentArrayMessageReader reader(serialized);
        reader.getRoot<Record>();
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "capnproto: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
boost_serialization_test(size_t iterations)
{
    using namespace boost_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::string serialized;

    to_string(r1, serialized);
    from_string(r2, serialized);

    if (r1 != r2) {
        throw std::logic_error("boost's case: deserialization failed");
    }

    std::cout << "boost: version = " << BOOST_VERSION << std::endl;
    std::cout << "boost: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized.clear();
        to_string(r1, serialized);
        from_string(r2, serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "boost: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
msgpack_serialization_test(size_t iterations)
{
    using namespace msgpack_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    msgpack::sbuffer sbuf;

    msgpack::pack(sbuf, r1);

    std::string serialized(sbuf.data(), sbuf.size());

    msgpack::unpacked msg;
    msgpack::unpack(&msg, serialized.data(), serialized.size());

    msgpack::object obj = msg.get();

    obj.convert(&r2);

    if (r1 != r2) {
        throw std::logic_error("msgpack's case: deserialization failed");
    }

    std::cout << "msgpack: version = " << msgpack_version() << std::endl;
    std::cout << "msgpack: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        sbuf.clear();
        msgpack::pack(sbuf, r1);
        msgpack::unpacked msg;
        msgpack::unpack(&msg, sbuf.data(), sbuf.size());
        msgpack::object obj = msg.get();
        obj.convert(&r2);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "msgpack: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
cereal_serialization_test(size_t iterations)
{
    using namespace cereal_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::string serialized;

    to_string(r1, serialized);
    from_string(r2, serialized);

    if (r1 != r2) {
        throw std::logic_error("cereal's case: deserialization failed");
    }

    std::cout << "cereal: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized.clear();
        to_string(r1, serialized);
        from_string(r2, serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "cereal: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
avro_serialization_test(size_t iterations)
{
    using namespace avro_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::auto_ptr<avro::OutputStream> out = avro::memoryOutputStream();
    avro::EncoderPtr encoder = avro::binaryEncoder();

    encoder->init(*out);
    avro::encode(*encoder, r1);

    auto serialized_size = out->byteCount();

    std::auto_ptr<avro::InputStream> in = avro::memoryInputStream(*out);
    avro::DecoderPtr decoder = avro::binaryDecoder();

    decoder->init(*in);
    avro::decode(*decoder, r2);

    if (r1.ids != r2.ids || r1.strings != r2.strings ||
        r2.ids.size() != kIntegers.size() || r2.strings.size() != kStringsCount) {
        throw std::logic_error("avro's case: deserialization failed");
    }

    std::cout << "avro: size = " << serialized_size << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        auto out = avro::memoryOutputStream();
        auto encoder = avro::binaryEncoder();
        encoder->init(*out);
        avro::encode(*encoder, r1);

        auto in = avro::memoryInputStream(*out);
        auto decoder = avro::binaryDecoder();
        decoder->init(*in);
        avro::decode(*decoder, r2);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "avro: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void hpx_serialization_test(size_t iterations)
{
    using namespace hpx_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::string serialized;

    to_string(r1, serialized);
    from_string(r2, serialized);

    if (r1 != r2) {
        throw std::logic_error("hpx's case: deserialization failed");
    }

    std::cout << "hpx: version = " << hpx::full_version_as_string() << std::endl;
    std::cout << "hpx: size    = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized.clear();
        to_string(r1, serialized);
        from_string(r2, serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "hpx: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void hpx_zero_copy_serialization_test(size_t iterations)
{
    using namespace hpx_zero_copy_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::string serialized;

    to_string(r1, serialized);
    from_string(r2, serialized);

    if (r1 != r2) {
        throw std::logic_error("hpx_zero_copy's case: deserialization failed");
    }

    std::cout << "hpx_zero_copy: version = " << hpx::full_version_as_string() << std::endl;
    std::cout << "hpx_zero_copy: size    = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        serialized.clear();
        to_string(r1, serialized);
        from_string(r2, serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "hpx_zero_copy: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void mpi_serialization_test(size_t iterations)
{
    using namespace mpi_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    int total_size = determine_pack_size(r1);
    std::string serialized(total_size, ' ');

    std::cout << "mpi: version = " << OMPI_MAJOR_VERSION << "."
              << OMPI_MINOR_VERSION << "." << OMPI_RELEASE_VERSION << std::endl;
    std::cout << "mpi: size    = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        to_string(r1, serialized);
        from_string(r2, serialized);
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "mpi: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
yas_serialization_test(size_t iterations)
{
    using namespace yas_test;

    Record r1, r2;

    for (size_t i = 0; i < kIntegers.size(); i++) {
        r1.ids.push_back(kIntegers[i]);
    }

    for (size_t i = 0; i < kStringsCount; i++) {
        r1.strings.push_back(kStringValue);
    }

    std::string serialized;

    to_string(r1, serialized);
    from_string(r2, serialized);

    if (r1 != r2) {
        throw std::logic_error("yas' case: deserialization failed");
    }

    std::cout << "yas: size = " << serialized.size() << " bytes" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        yas::mem_ostream os;
        yas::binary_oarchive<yas::mem_ostream> oa(os);
        oa & r1;

        yas::mem_istream is(os.get_intrusive_buffer());
        yas::binary_iarchive<yas::mem_istream> ia(is);
        ia & r2;
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "yas: time = " << duration << " milliseconds" << std::endl << std::endl;
}

void
flatbuffers_serialization_test(size_t iterations)
{
    using namespace flatbuffers_test;

    std::vector<flatbuffers::Offset<flatbuffers::String>> strings;
    strings.reserve(kStringsCount);

    flatbuffers::FlatBufferBuilder builder;
    for (size_t i = 0; i < kStringsCount; i++) {
        strings.push_back(builder.CreateString(kStringValue));
    }

    auto ids_vec = builder.CreateVector(kIntegers);
    auto strings_vec = builder.CreateVector(strings);
    auto r1 = CreateRecord(builder, ids_vec, strings_vec);

    builder.Finish(r1);

    auto p = reinterpret_cast<char*>(builder.GetBufferPointer());
    auto sz = builder.GetSize();
    std::vector<char> buf(p, p + sz);

    auto r2 = GetRecord(buf.data());
    if (r2->strings()->size() != kStringsCount || r2->ids()->size() != kIntegers.size()) {
        throw std::logic_error("flatbuffer's case: deserialization failed");
    }

    std::cout << "flatbuffers: version = " << FLATBUFFERS_VERSION_MAJOR << "."
              << FLATBUFFERS_VERSION_MINOR << "." << FLATBUFFERS_VERSION_REVISION << std::endl;
    std::cout << "flatbuffers: size    = " << builder.GetSize() << " bytes" << std::endl;

    builder.ReleaseBufferPointer();

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; i++) {
        builder.Clear();
        strings.clear();
        // buf.clear();

        for (size_t i = 0; i < kStringsCount; i++) {
            strings.push_back(builder.CreateString(kStringValue));
        }

        auto ids_vec = builder.CreateVector(kIntegers);
        auto strings_vec = builder.CreateVector(strings);
        auto r1 = CreateRecord(builder, ids_vec, strings_vec);
        builder.Finish(r1);

        auto p = reinterpret_cast<char*>(builder.GetBufferPointer());
        auto sz = builder.GetSize();
        std::vector<char> buf(p, p + sz);
        auto r2 = GetRecord(buf.data());
        (void)r2->ids()[0];

        builder.ReleaseBufferPointer();
    }
    auto finish = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count();

    std::cout << "flatbuffers: time = " << duration << " milliseconds" << std::endl << std::endl;
}

int
main(int argc, char **argv)
{
#ifdef WITH_MPI
    MPI_Init(&argc, &argv);
#endif

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (argc < 2) {
        std::cout << "usage: " << argv[0] << " N [thrift-binary thrift-compact protobuf boost msgpack cereal avro hpx capnproto flatbuffers yas]";
        std::cout << std::endl << std::endl;
        std::cout << "arguments: " << std::endl;
        std::cout << " N  -- number of iterations" << std::endl << std::endl;
        return EXIT_SUCCESS;
    }

    size_t iterations;

    try {
        iterations = boost::lexical_cast<size_t>(argv[1]);
    } catch (std::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        std::cerr << "First positional argument must be an integer." << std::endl;
        return EXIT_FAILURE;
    }

    std::set<std::string> names;

    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            names.insert(argv[i]);
        }
    }

    std::cout << "performing " << iterations << " iterations" << std::endl << std::endl;

    /*std::cout << "total size: " << sizeof(kIntegerValue) * kIntegersCount + kStringValue.size() * kStringsCount << std::endl;*/

    try {
        if (names.empty() || names.find("thrift-binary") != names.end()) {
            thrift_serialization_test(iterations, ThriftSerializationProto::Binary);
        }

        if (names.empty() || names.find("thrift-compact") != names.end()) {
            thrift_serialization_test(iterations, ThriftSerializationProto::Compact);
        }

        if (names.empty() || names.find("protobuf") != names.end()) {
            protobuf_serialization_test(iterations);
        }

        if (names.empty() || names.find("capnproto") != names.end()) {
            capnproto_serialization_test(iterations);
        }

        if (names.empty() || names.find("boost") != names.end()) {
            boost_serialization_test(iterations);
        }

        if (names.empty() || names.find("msgpack") != names.end()) {
            msgpack_serialization_test(iterations);
        }

        if (names.empty() || names.find("cereal") != names.end()) {
            cereal_serialization_test(iterations);
        }

        if (names.empty() || names.find("avro") != names.end()) {
            avro_serialization_test(iterations);
        }

        if (names.empty() || names.find("hpx") != names.end()) {
            hpx_serialization_test(iterations);
        }

        if (names.empty() || names.find("hpx_zero_copy") != names.end()) {
            hpx_zero_copy_serialization_test(iterations);
        }

#ifdef WITH_MPI
        if (names.empty() || names.find("mpi") != names.end()) {
            mpi_serialization_test(iterations);
        }
#endif
        if (names.empty() || names.find("yas") != names.end()) {
            yas_serialization_test(iterations);
        }

        if (names.empty() || names.find("flatbuffers") != names.end()) {
            flatbuffers_serialization_test(iterations);
        }
    } catch (std::exception &exc) {
        std::cerr << "Error: " << exc.what() << std::endl;
        return EXIT_FAILURE;
    }

    google::protobuf::ShutdownProtobufLibrary();

#ifdef WITH_MPI
    MPI_Finalize();
#endif
    return EXIT_SUCCESS;
}
