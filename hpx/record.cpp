#include "hpx/record.hpp"

namespace hpx_test {

void
to_string(const Record &record, std::string& data)
{
    hpx::serialization::output_archive archiver(data);
    archiver << record;
}

void
from_string(Record &record, const std::string& data)
{
    hpx::serialization::input_archive archiver(data);
    archiver >> record;
}

} // namespace
