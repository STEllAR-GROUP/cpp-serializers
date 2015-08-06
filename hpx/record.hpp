#ifndef __HPX_RECORD_HPP_INCLUDED__
#define __HPX_RECORD_HPP_INCLUDED__

#include <vector>
#include <string>
#include <sstream>

#include <stdint.h>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/string.hpp>
#include <hpx/runtime/serialization/vector.hpp>

namespace hpx_test {

typedef std::vector<int64_t>     Integers;
typedef std::vector<std::string> Strings;

class Record {
public:

    Integers ids;
    Strings  strings;

    bool operator==(const Record &other) {
        return (ids == other.ids && strings == other.strings);
    }

    bool operator!=(const Record &other) {
        return !(*this == other);
    }

private:
    friend class hpx::serialization::access;

    template<typename Archive>
    void serialize(Archive &ar, unsigned int)
    {
        ar & ids;
        ar & strings;
    }
};

void to_string(const Record &record, std::string& data);
void from_string(Record &record, const std::string& data);

} // namespace

#endif
