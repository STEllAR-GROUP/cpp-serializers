#ifndef __MPI_RECORD_HPP_INCLUDED__
#define __MPI_RECORD_HPP_INCLUDED__

#include <vector>
#include <string>
#include <sstream>

#include <stdint.h>

namespace mpi_test {

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
};

int determine_pack_size(const Record &record)
{
    int total_size = 0;
    int partial_size;

    int num_ints = record.ids.size();
    MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &partial_size);
    total_size += partial_size;

    MPI_Pack_size(record.ids.size(), MPI_INT, MPI_COMM_WORLD, &partial_size);
    total_size += partial_size;

    MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &partial_size);
    total_size += partial_size;

    for (size_t i = 0; i < record.strings.size(); i++) {
        int length = record.strings[i].size();
        MPI_Pack_size(1, MPI_INT, MPI_COMM_WORLD, &partial_size);
        total_size += partial_size;

        MPI_Pack_size(length, MPI_CHAR, MPI_COMM_WORLD, &partial_size);
        total_size += partial_size;
    }

    return total_size;
}

void to_string(const Record &record, std::string& data)
{
    char *data_ptr = &data[0];
    int position = 0;

    int num_ints = record.ids.size();
    MPI_Pack(&num_ints, 1, MPI_INT,
             data_ptr, data.size(), &position, MPI_COMM_WORLD);

    MPI_Pack(&record.ids[0], record.ids.size(), MPI_INT,
             data_ptr, data.size(), &position, MPI_COMM_WORLD);

    int num_strings = record.strings.size();
    MPI_Pack(&num_strings, 1, MPI_INT,
             data_ptr, data.size(), &position, MPI_COMM_WORLD);

    for (size_t i = 0; i < record.strings.size(); i++) {

        int length = record.strings[i].size();
        MPI_Pack(&length, 1, MPI_INT,
                 data_ptr, data.size(), &position, MPI_COMM_WORLD);

        MPI_Pack(&record.strings[i][0], record.strings[i].size(), MPI_CHAR,
                 data_ptr, data.size(), &position, MPI_COMM_WORLD);
    }
}

void from_string(Record &record, const std::string& data)
{
    char *data_ptr = const_cast<char*>(&data[0]);
    int position = 0;

    int num_ints;
    MPI_Unpack(data_ptr, data.size(), &position,
               &num_ints, 1, MPI_INT,
               MPI_COMM_WORLD);

    record.ids.resize(num_ints);
    MPI_Unpack(data_ptr, data.size(), &position,
               &record.ids[0], num_ints, MPI_INT,
               MPI_COMM_WORLD);

    int num_strings;
    MPI_Unpack(data_ptr, data.size(), &position,
               &num_strings, 1, MPI_INT,
               MPI_COMM_WORLD);

    record.strings.resize(num_strings);
    for (size_t i = 0; i < num_strings; i++) {
        int length;
        MPI_Unpack(data_ptr, data.size(), &position,
                   &length, 1, MPI_INT,
                   MPI_COMM_WORLD);

        record.strings[i].resize(length);
        MPI_Unpack(data_ptr, data.size(), &position,
                   &record.strings[i][0], length, MPI_CHAR,
                   MPI_COMM_WORLD);
    }
}

} // namespace

#endif
