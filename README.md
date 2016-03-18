#### [Thrift](http://thrift.apache.org/) vs. [Protobuf](https://code.google.com/p/protobuf/) vs. [Boost.Serialization](http://www.boost.org/libs/serialization) vs. [Msgpack](http://msgpack.org/) vs. [Cereal](http://uscilab.github.io/cereal/index.html) vs. [Avro](http://avro.apache.org/) vs. [HPX](https://github.com/STEllAR-GROUP/hpx) serialization/deserialization time test for C++.

#### Build
This project does not have any external library dependencies. All (boost, thrift etc.) needed libraries are downloaded
and built automatically except HPX (set HPX_DIR for latter), but you need enough free disk space to build all components. To build this project you need a compiler that supports
C++11 features. Project was tested with GCC 4.9.1 (Ubuntu 14.04).

```
$ git clone https://github.com/thekvs/cpp-serializers.git
$ mkdir /path/to/build-root/
$ cd /path/to/build-root/
$ cmake /path/to/cpp-serializers -DCMAKE_BUILD_TYPE=Release -DHPX_DIR=...
$ make
```

#### Usage
* Test __all__ serializers, run each serializer 100000 times:
```
$ ./test 100000
```
* Test only __protobuf__ serializer, run it 100000 times:
```
$ ./test 100000 protobuf
```
* Test __protobuf__ and __cereal__ serializers only, run each of them 100000 times:
```
$ ./test 100000 protobuf cereal
```

#### Results

Following results were obtained running 1000000 serialize-deserialize operations 50 times and then averaging results
on a typical desktop computer with Intel Xeon CPU E5-2450 processor running Debian. Compiler was gcc-6.0 (from trunk). Exact versions of libraries used are:

* thrift 0.9.3
* protobuf 2.6.0
* boost 1.60.0
* msgpack 0.5.9
* cereal 1.1.2
* avro 1.7.7
* hpx 0.9.12

| serializer     | object's size | avg. total time |
| -------------- | ------------- | --------------- |
| thrift-binary  | 17017         | 28025           |
| thrift-compact | 11597         | 33802           |
| protobuf       | 12571         | 23843           |
| boost          | 17470         | 16188           |
| msgpack        | 11902         | 30051           |
| cereal         | 17416         | 13083           |
| avro           | 12288         | 38225           |
| hpx            | 17433         | 12371           |
| hpx_zero_copy  | 9433          | 8138            |

Size measured in bytes, time measured in milliseconds.

##### Graphical representations

###### Size

![Size](images/size-hpx.png)

###### Time

![Time](images/time-hpx.png)
