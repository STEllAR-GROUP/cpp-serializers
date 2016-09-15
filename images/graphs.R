library(ggplot2)

names <- c(
     "thrift-binary"
    ,"thrift-compact"
    ,"protobuf"
    ,"capnproto"
    ,"boost"
    ,"msgpack"
    ,"flatbuffers"
    ,"cereal"
    ,"avro"
    ,"hpx"
    ,"hpx_zero_copy"
    ,"mpi"
    ,"yas"
)

# for t in thrift-binary thrift-compact protobuf capnproto boost msgpack flatbuffers cereal avro hpx hpx_zero_copy mpi yas; do echo -n "$t: "; ./test 1 $t | grep size | awk '{print $4}'; done
size <- c(
     17017 # thrift-binary
    ,11597 # thrift-compact
    ,12571 # protobuf
    ,17768 # capnproto
    ,17470 # boost
    ,11802 # msgpack
    ,17632 # flatbuffers
    ,17416 # cereal
    ,12288 # avro
    ,17433 # hpx
    ,9433  # hpx_zero_copy
    ,13008 # mpi
    ,17012 # yas
)

# data from the 1000000 simulations
# for t in in thrift-binary thrift-compact protobuf capnproto boost msgpack flatbuffers cereal avro hpx hpx_zero_copy mpi yas; do rm -f /tmp/$t.time; echo -n "$t: "; for i in `seq 1 50`; do ./benchmark 1000000 $t | grep time | awk '{print $4}' >>/tmp/$t.time; done; awk '{ sum += $1 } END { print sum/50}' /tmp/$t.time; done
time <- c(
     21653 # thrift-binary
    ,26672 # thrift-compact
    ,19314 # protobuf
    ,33    # capnproto
    ,12046 # boost
    ,26275 # msgpack
    ,12381 # flatbuffers
    ,10073 # cereal
    ,29853 # avro
    ,12339 # hpx
    ,8140  # hpx_zero_copy
    ,7924  # mpi
    ,4210  # yas
)

data.size <- as.data.frame(list(serializer = names, size = size))
data.time <- as.data.frame(list(serializer = names, time = time))

p0 <- ggplot(data.size, aes(x = as.factor(serializer), y = as.factor(size), fill = serializer)) +
    geom_bar(stat = "identity") +
    xlab("") +
    ylab("size in bytes") +
    theme(axis.text.x = element_text(angle = 90, hjust = 1))
png(filename="size.png", width = 800, height = 600)
plot(p0)
dev.off()

p1 <- ggplot(data.time, aes(x = as.factor(serializer), y = as.factor(time), fill = serializer)) +
    geom_bar(stat = "identity") +
    xlab("") +
    ylab("time in ms") +
    theme(axis.text.x = element_text(angle = 90, hjust = 1))
png(filename="time.png", width = 800, height = 600)
plot(p1)
dev.off()
