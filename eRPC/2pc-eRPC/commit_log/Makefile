CXX = g++
export ROCKSDB_DIR = /home/dimitra/workspace/rocksdb-5.6-native_scone
CFLAGS = -Wall -g -std=c++17 -I../third_party/openssl/include -I../advanced_encryption_library -I../rocksdb/include -I../rocksdb -I../ -I../rocksdb/util -I../lib -I../recovery -DROCKSDB_PLATFORM_POSIX -DNDEBUG -I${ROCKSDB_DIR}/include -I${ROCKSDB_DIR} -I${ROCKSDB_DIR}/util -DENCRYPTION

LFLAGS = -L${ROCKSDB_DIR} -L../third_party/openssl/lib
LDFLAGS = -lrocksdb -lpthread -ldl -lrt -lcrypto
.PHONY = all clean

all: clean encrypted_clog

construct_clog: clean
	$(CXX) -o $@ $(CFLAGS) ../src/txn_phases.cc clog.cc construct_artificial_clogs.cc ${LFLAGS} ${LDFLAGS}

clog_test:
	$(CXX) -o log_test $(CFLAGS) ../src/txn_phases.cc clog.cc log_test.cc ${LFLAGS} ${LDFLAGS}

encrypted_clog: clean
	$(CXX) -o $@ $(CFLAGS) ../src/txn_phases.cc clog.cc encrypted_clog.cc ${LFLAGS} ${LDFLAGS}

clean:
	rm -f log_test construct_clog encrypted_clog
	rm -f testLog.txt
