#pragma once

#include <string>
#include <cstring>
#include <cassert>


#include "util/file_reader_writer.h"
#include "rocksdb/env.h"
#include "rocksdb/slice.h"
#include "db/log_reader.h"
#include "db/log_writer.h"
#include "port/port.h"

#include "txn_phases.h"

#include "encrypt_package.h"


class CommitLog {
	public:
		CommitLog() = delete;
		~CommitLog();

		CommitLog(std::shared_ptr<PacketSsl> packet, std::string _fname, bool _manual_flush = false, bool _checksum_ = false);
		CommitLog(std::string _fname, bool _manual_flush = false, bool _checksum_ = false);

		// no copy-allowed
		CommitLog(const CommitLog& other) = delete; 
		void operator=(const CommitLog& other) = delete; 

		// no move semantics
		CommitLog(CommitLog&& other) = delete; 
		void operator=(CommitLog&& other) = delete; 

		bool AppendToCLog(int RID, int tCoordinator, int globalTxn, int phase, int decision, int kvPairsNumber = 0, const char* _data = nullptr, size_t data_size = 0);

	private:
		std::string fname;
		bool const manual_flush, checksum_;

		std::unique_ptr<rocksdb::WritableFile> lfile;
		rocksdb::Env* env_;

		rocksdb::Status s;

		std::shared_ptr<PacketSsl> packet;

		// rocksdb::log::Reader* clogReader;
#ifdef SCONE
		rocksdb::log::Writer<rocksdb::log::WriterBaseWAL>* clogWriter;
#else
		rocksdb::log::Writer* clogWriter;
#endif
};


class CommitLogReader {
	public:
		CommitLogReader() = delete;
		~CommitLogReader();

		CommitLogReader(std::string _fname);
		CommitLogReader(std::shared_ptr<PacketSsl> packet, std::string _fname);

		// no copy-allowed
		CommitLogReader(const CommitLogReader& other) = delete; 
		void operator=(const CommitLogReader& other) = delete; 

		// no move semantics
		CommitLogReader(CommitLogReader&& other) = delete; 
		void operator=(CommitLogReader&& other) = delete; 

		bool RecoverLog(int& RID, int& globId, int& tc, int& phase, std::unordered_map<std::string, std::string>& _map);

		bool RecoverLogEntry(int& RID, int& globId, int& tc, int& phase, std::unordered_map<std::string, std::string>& _map);

		void decodeRecord(rocksdb::Slice& record, int& RID, int& globId, int& tc, int& phase, int& decision, std::unordered_map<std::string, std::string>& _map);
	private:
		std::string fname;

		std::unique_ptr<rocksdb::SequentialFile> lfile;
		rocksdb::Env* env_;
		std::shared_ptr<PacketSsl> packet;

		rocksdb::Status s;
#ifndef SCONE
		rocksdb::log::Reader* clogReader;
#endif

#ifdef SCONE
		rocksdb::log::Reader<rocksdb::log::ReaderBaseWAL>* clogReader;
#endif
};
