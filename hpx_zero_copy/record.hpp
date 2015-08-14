#ifndef __HPX_ZERO_COPY_RECORD_HPP_INCLUDED__
#define __HPX_ZERO_COPY_RECORD_HPP_INCLUDED__

#include <vector>
#include <string>
#include <sstream>

#include <stdint.h>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/string.hpp>
#include <hpx/runtime/serialization/array.hpp>

namespace hpx_zero_copy_test {

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
    void save(Archive &ar, unsigned int) const
    {
        ar & ids.size();
        ar & hpx::serialization::make_array(ids.data(), ids.size());

        ar & strings.size();
        ar & hpx::serialization::make_array(strings.data(), strings.size());
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        {
            std::vector<int>::size_type size;
            ar & size; ids.resize(size);
            ar & hpx::serialization::make_array(ids.data(), ids.size());
        }

        {
            std::vector<std::string>::size_type size;
            ar & size; strings.resize(size);
            ar & hpx::serialization::make_array(strings.data(), strings.size());
        }
    }
    HPX_SERIALIZATION_SPLIT_MEMBER();
};

void to_string(const Record &record, std::string& data);
void from_string(Record &record, const std::string& data);

} // namespace

#endif
