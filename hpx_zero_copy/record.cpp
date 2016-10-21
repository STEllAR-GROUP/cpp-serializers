#include "hpx_zero_copy/record.hpp"

namespace hpx_zero_copy_test {

static std::vector<hpx::serialization::serialization_chunk> chunks;

void
to_string(const Record &record, std::string& data)
{
    chunks.clear();
    hpx::serialization::output_archive archiver(data, 0, &chunks);
    archiver << record;
}

void
from_string(Record &record, const std::string& data)
{
    hpx::serialization::input_archive archiver(data, 0u, &chunks);
    archiver >> record;
}

} // namespace
