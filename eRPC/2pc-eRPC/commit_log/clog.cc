#include "clog.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace rocksdb;

CommitLog::CommitLog(std::shared_ptr<PacketSsl> _packet, std::string _fname, bool _manual_flush, bool _checksum) : 
	fname(_fname),
	manual_flush(_manual_flush), 
	checksum_(_checksum) {
		packet = _packet;

		env_ = Env::Default();
		DBOptions db_options;
		EnvOptions opt_file_options = env_->OptimizeForLogWrite(EnvOptions(), db_options);
		std::string log_fname = _fname;
		s = env_->NewWritableFile(log_fname, &lfile, opt_file_options);

		if (s.ok()) {
			size_t preallocate_block_size = 218301;
			lfile->SetPreallocationBlockSize(preallocate_block_size);


			std::unique_ptr<WritableFileWriter> file_writer(
					new WritableFileWriter(std::move(lfile), opt_file_options));

#ifdef SCONE
			clogWriter = new log::Writer<rocksdb::log::WriterBaseWAL>(std::move(file_writer), 50, false);
#else
			clogWriter = new log::Writer(std::move(file_writer), 50, false);
#endif
		}
	};

CommitLog::CommitLog(std::string _fname, bool _manual_flush, bool _checksum) : 
	fname(_fname),
	manual_flush(_manual_flush), 
	checksum_(_checksum) {

		env_ = Env::Default();
		DBOptions db_options;
		EnvOptions opt_file_options = env_->OptimizeForLogWrite(EnvOptions(), db_options);
		std::string log_fname = _fname;
		s = env_->NewWritableFile(log_fname, &lfile, opt_file_options);

		if (s.ok()) {
			size_t preallocate_block_size = 218301;
			lfile->SetPreallocationBlockSize(preallocate_block_size);


			std::unique_ptr<WritableFileWriter> file_writer(
					new WritableFileWriter(std::move(lfile), opt_file_options));

#ifndef SCONE
			clogWriter = new log::Writer(std::move(file_writer), 50, false);
#endif
#ifdef SCONE
			clogWriter = new log::Writer<rocksdb::log::WriterBaseWAL>(std::move(file_writer), 50, false);
#endif

		}
	};


CommitLog::~CommitLog() {
	// DIMITRA delete clogWriter;
};

#ifdef ENCRYPTION
bool CommitLog::AppendToCLog(int RID, int tCoordinator, int globalTxn, int phase, int decision, int kvPairsNumber, const char* _data, size_t data_size) {
	Status s;
	size_t buf_size, offset;
	char* _buf;
	switch (phase) {
		case PRE_PREPARE:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": PRE_PREPARE\n";
				buf_size = 3*8 + 8 + data_size + 1 + 8;
				offset = 0;

				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", kvPairsNumber);
				::memcpy(_buf+offset, _data, data_size);
				offset += data_size;

				_buf[buf_size - 1] = '\0';

				auto *dst_buf = new uint8_t[PacketSsl::get_buffer_size(buf_size)];
				packet->encrypt(dst_buf, _buf, buf_size);

				Slice data(reinterpret_cast<char*>(dst_buf), PacketSsl::get_buffer_size(buf_size));

				s = clogWriter->AddRecord(data);
				delete[] dst_buf;
			}
			break;

		case POST_PREPARE:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": POST_PREPARE\n";
				buf_size = 3*8 + 8 + 1 + 8;
				offset = 0;
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 

				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", decision);
				_buf[buf_size -1] = '\0';

				auto *dst_buf = new uint8_t[PacketSsl::get_buffer_size(buf_size)];
				packet->encrypt(dst_buf, _buf, buf_size);

				Slice data(reinterpret_cast<char*>(dst_buf), PacketSsl::get_buffer_size(buf_size));

				s = clogWriter->AddRecord(data);
				delete[] dst_buf;
			}
			break;

		case PARTICIPANTS_PREPARE:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": PARTICIPANTS_PREPARE\n";
				buf_size = 3*8 + 8 + 8 + data_size + 1 + 8;
				offset = 0;
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 

				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", decision);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", kvPairsNumber);
				::memcpy(_buf+offset, _data, data_size);
				offset += data_size;
				_buf[buf_size -1] = '\0';


				auto *dst_buf = new uint8_t[PacketSsl::get_buffer_size(buf_size)];
				packet->encrypt(dst_buf, _buf, buf_size);

				Slice data(reinterpret_cast<char*>(dst_buf), PacketSsl::get_buffer_size(buf_size));

				s = clogWriter->AddRecord(data);
				delete[] dst_buf;
			}
			break;

		case COMMIT:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": COMMIT\n";
				// buf_size = 3*8 + 8 + data_size + 1;
				buf_size = 3*8 + 8 + 1 + 8;
				offset = 0;
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 

				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", decision);
				_buf[buf_size -1] = '\0';

				auto *dst_buf = new uint8_t[PacketSsl::get_buffer_size(buf_size)];
				packet->encrypt(dst_buf, _buf, buf_size);

				Slice data(reinterpret_cast<char*>(dst_buf), PacketSsl::get_buffer_size(buf_size));
				s = clogWriter->AddRecord(data);
				delete[] dst_buf;
			}
			break;

		default:
			assert(false);
	}
	// clogWriter->WriteBuffer(); // only if manual_flush is set to 'true'

	return s.ok() ? true : false;
};
#else
bool CommitLog::AppendToCLog(int RID, int tCoordinator, int globalTxn, int phase, int decision, int kvPairsNumber, const char* _data, size_t data_size) {
	Status s;
	size_t buf_size, offset;
	char* _buf;
	switch (phase) {
		case PRE_PREPARE:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": PRE_PREPARE\n";
				buf_size = 3*8 + 8 + data_size + 1 + 8;
				offset = 0;
				// std::unique_ptr<char[]> buf(new char[buf_size]);
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 
				// _buf = new char[buf_size];
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", kvPairsNumber);
				::memcpy(_buf+offset, _data, data_size);
				offset += data_size;

				_buf[buf_size - 1] = '\0';

				Slice data(_buf, buf_size);
				// std::cout << __PRETTY_FUNCTION__ << " --- " << data.ToString() << "\n";

				s = clogWriter->AddRecord(data);

				// delete[] _buf;
			}
			break;

		case POST_PREPARE:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": POST_PREPARE\n";
				buf_size = 3*8 + 8 + 1 + 8;
				offset = 0;
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 

				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", decision);
				_buf[buf_size -1] = '\0';

				Slice data(buf.get(), buf_size);
				// std::cout << __PRETTY_FUNCTION__ << " " << data.ToString() << "\n";

				s = clogWriter->AddRecord(data);
			}
			break;

		case PARTICIPANTS_PREPARE:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": PARTICIPANTS_PREPARE\n";
				buf_size = 3*8 + 8 + 8 + data_size + 1 + 8;
				offset = 0;
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 

				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", decision);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", kvPairsNumber);
				::memcpy(_buf+offset, _data, data_size);
				offset += data_size;
				_buf[buf_size -1] = '\0';

				Slice data(buf.get(), buf_size);
				// std::cout << __PRETTY_FUNCTION__ << " " << data.ToString() << "\n";

				s = clogWriter->AddRecord(data);
			}
			break;

		case COMMIT:
			{
				// std::cout << __PRETTY_FUNCTION__ << ": COMMIT\n";
				// buf_size = 3*8 + 8 + data_size + 1;
				buf_size = 3*8 + 8 + 1 + 8;
				offset = 0;
				std::unique_ptr<char[]> buf(new char[buf_size]);

				// _buf is a raw ptr to char[] but the ownership is 
				// still managed by the unique ptr! Do not 'delete _buf' or 
				// transfer ownership (create another unique_ptr through _buf)!
				_buf = buf.get(); // _buf is a raw ptr to char[] but the ownership is 

				offset += snprintf(_buf+offset, buf_size-offset, "%08d", RID);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", tCoordinator);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", globalTxn);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", phase);
				offset += snprintf(_buf+offset, buf_size-offset, "%08d", decision);
				_buf[buf_size -1] = '\0';

				Slice data(buf.get(), buf_size);
				// std::cout << __PRETTY_FUNCTION__ << " " << data.ToString() << "\n";

				s = clogWriter->AddRecord(data);
			}
			break;

		default:
			assert(false);
	}
	// clogWriter->WriteBuffer(); // only if manual_flush is set to 'true'

	return s.ok() ? true : false;
};
#endif

CommitLogReader::CommitLogReader(std::string fname) {

	env_ = Env::Default();
	s = env_->NewSequentialFile(fname, &lfile, EnvOptions());
	assert(s.ok());

	env_->OptimizeForLogRead(EnvOptions());
	size_t log_readahead_size = 0;

	std::unique_ptr<SequentialFileReader> file_reader(new SequentialFileReader(std::move(lfile)));


	// Create the log reader.
#ifndef SCONE
	clogReader = new log::Reader(nullptr, std::move(file_reader), nullptr, true, 0, 50);
#endif


#ifdef SCONE
//DIMITRA	clogReader = new log::Reader<rocksdb::log::ReaderBaseWAL>(std::move(file_reader), 50, false);
#endif

	// assert(clogReader != nullptr);
};



CommitLogReader::CommitLogReader(std::shared_ptr<PacketSsl> _packet, std::string fname) {
	packet = _packet;

	env_ = Env::Default();
	s = env_->NewSequentialFile(fname, &lfile, EnvOptions());
	assert(s.ok());

	env_->OptimizeForLogRead(EnvOptions());
	size_t log_readahead_size = 0;

	std::unique_ptr<SequentialFileReader> file_reader(new SequentialFileReader(std::move(lfile)));


	// Create the log reader.
#ifndef SCONE
	clogReader = new log::Reader(nullptr, std::move(file_reader), nullptr, true, 0, 50);
#endif
	//DIMITRA assert(clogReader != nullptr);
};


#ifdef ENCRYPTION
void CommitLogReader::decodeRecord(rocksdb::Slice& record, int& remoteID, int& globId, int& tc, int& phase, int& decision, std::unordered_map<std::string, std::string>& _map) {
	char coordinator[9], txnId[9], ph[9], RID[9];

	size_t msg_size = record.size();
	auto *dec_data = new uint8_t[msg_size];

	bool ok = packet->decrypt(dec_data, record.data(), msg_size);
	if(!ok)
		exit(1);
	assert(ok);

	// std::cout << dec_data << "\n";
	char* slice = reinterpret_cast<char*>(dec_data);

	::memcpy(RID, slice, 8);
	::memcpy(coordinator, slice + 8, 8);
	::memcpy(txnId, slice + 8 + 8, 8);
	::memcpy(ph, slice + 8 + 8 + 8, 8);
	RID[8] = '\0';
	coordinator[8] = '\0';
	txnId[8] = '\0';
	ph[8] = '\0';

	globId = std::atoi(txnId);
	tc = std::atoi(coordinator);
	phase = std::atoi(ph);
	remoteID = std::atoi(RID);

	switch (phase) {
		case PRE_PREPARE:		/* coordinator's related phase */
			{

				char kvPairsNumber[9];
				size_t offset = 8 + 8 + 8 + 8;
				::memcpy(kvPairsNumber, slice + offset, 8);
				kvPairsNumber[8] = '\0';
				offset += 8;
				int pairs = std::atoi(kvPairsNumber);
				// std::cout << "KV-Pairs " << pairs << "\n";
				int key_size, val_size;
				char _key_size[9], _val_size[9];
				std::unique_ptr<char[]> key;
				std::unique_ptr<char[]> value;
				for (int i = 0; i < pairs; i++) {
					::memcpy(_key_size, slice + offset, 8);
					offset += 8;
					::memcpy(_val_size, slice + offset, 8);
					offset += 8;
					_key_size[8] = '\0';
					_val_size[8] = '\0';
					key_size = std::atoi(_key_size);
					val_size = std::atoi(_val_size);

#ifdef DEBUG_EXTRA
					//	std::cout << key_size << " " << val_size << "\n";
#endif

					key.reset(new char[key_size+1]);
					value.reset(new char[val_size+1]);

					::memcpy(key.get(), slice + offset, key_size);
					key.get()[key_size] = '\0';
					offset += key_size;

					::memcpy(value.get(), slice + offset, val_size);
					value.get()[val_size] = '\0';
					offset += val_size;

					//	std::cout << __PRETTY_FUNCTION__ << " " << key_size << " " << val_size << "\n";
					//	std::cout << __PRETTY_FUNCTION__ << " " << key.get() << " " << value.get() << "\n";
					_map.insert(std::make_pair(std::string(key.get(), key_size), std::string(value.get(), val_size)));
				}


			}
			break;
		case POST_PREPARE:		/* coordinator's related phase */
			{
				char des[9];
				::memcpy(des, slice + 8 + 8 + 8 + 8, 8);
				decision = std::atoi(des);

			}
			break;
		case PARTICIPANTS_PREPARE:	/* participants */
			{
				char des[9];
				::memcpy(des, slice + 8 + 8 + 8 + 8, 8);
				decision = std::atoi(des);

				char kvPairsNumber[9];
				size_t offset = 8 + 8 + 8 + 8 + 8;
				::memcpy(kvPairsNumber, slice + offset, 8);
				kvPairsNumber[8] = '\0';
				offset += 8;
				int pairs = std::atoi(kvPairsNumber);
				// std::cout << "KV-Pairs " << pairs << "\n";
				int key_size, val_size;
				char _key_size[9], _val_size[9];
				std::unique_ptr<char[]> key;
				std::unique_ptr<char[]> value;
				for (int i = 0; i < pairs; i++) {
					::memcpy(_key_size, slice + offset, 8);
					offset += 8;
					::memcpy(_val_size, slice + offset, 8);
					offset += 8;
					_key_size[8] = '\0';
					_val_size[8] = '\0';
					key_size = std::atoi(_key_size);
					val_size = std::atoi(_val_size);


					key.reset(new char[key_size+1]);
					value.reset(new char[val_size+1]);

					::memcpy(key.get(), slice + offset, key_size);
					key.get()[key_size] = '\0';
					offset += key_size;

					::memcpy(value.get(), slice + offset, val_size);
					value.get()[val_size] = '\0';
					offset += val_size;

					_map.insert(std::make_pair(std::string(key.get(), key_size), std::string(value.get(), val_size)));
				}
			}
			break;
		case COMMIT:
			char des[9];
			::memcpy(des, slice + 8 + 8 + 8 + 8, 8);
			decision = std::atoi(des);
			break;
		default:
			assert(false);
	}
}
#else

void CommitLogReader::decodeRecord(rocksdb::Slice& record, int& remoteID, int& globId, int& tc, int& phase, int& decision, std::unordered_map<std::string, std::string>& _map) {
	char coordinator[9], txnId[9], ph[9], RID[9];
	const char* slice = record.data();

	::memcpy(RID, slice, 8);
	::memcpy(coordinator, slice + 8, 8);
	::memcpy(txnId, slice + 8 + 8, 8);
	::memcpy(ph, slice + 8 + 8 + 8, 8);
	RID[8] = '\0';
	coordinator[8] = '\0';
	txnId[8] = '\0';
	ph[8] = '\0';

	globId = std::atoi(txnId);
	tc = std::atoi(coordinator);
	phase = std::atoi(ph);
	remoteID = std::atoi(RID);

	switch (phase) {
		case PRE_PREPARE:		/* coordinator's related phase */
			{

				char kvPairsNumber[9];
				size_t offset = 8 + 8 + 8 + 8;
				::memcpy(kvPairsNumber, slice + offset, 8);
				kvPairsNumber[8] = '\0';
				offset += 8;
				int pairs = std::atoi(kvPairsNumber);
				// std::cout << "KV-Pairs " << pairs << "\n";
				int key_size, val_size;
				char _key_size[9], _val_size[9];
				std::unique_ptr<char[]> key;
				std::unique_ptr<char[]> value;
				for (int i = 0; i < pairs; i++) {
					::memcpy(_key_size, slice + offset, 8);
					offset += 8;
					::memcpy(_val_size, slice + offset, 8);
					offset += 8;
					_key_size[8] = '\0';
					_val_size[8] = '\0';
					key_size = std::atoi(_key_size);
					val_size = std::atoi(_val_size);

#ifdef DEBUG_EXTRA & 0
					std::cout << key_size << " " << val_size << "\n";
#endif

					key.reset(new char[key_size+1]);
					value.reset(new char[val_size+1]);

					::memcpy(key.get(), slice + offset, key_size);
					key.get()[key_size] = '\0';
					offset += key_size;

					::memcpy(value.get(), slice + offset, val_size);
					value.get()[val_size] = '\0';
					offset += val_size;

					_map.insert(std::make_pair(std::string(key.get(), key_size), std::string(value.get(), val_size)));
				}


			}
			break;
		case POST_PREPARE:		/* coordinator's related phase */
			{
				char des[9];
				::memcpy(des, slice + 8 + 8 + 8 + 8, 8);
				decision = std::atoi(des);

			}
			break;
		case PARTICIPANTS_PREPARE:	/* participants */
			{
				char des[9];
				::memcpy(des, slice + 8 + 8 + 8 + 8, 8);
				decision = std::atoi(des);

				char kvPairsNumber[9];
				size_t offset = 8 + 8 + 8 + 8 + 8;
				::memcpy(kvPairsNumber, slice + offset, 8);
				kvPairsNumber[8] = '\0';
				offset += 8;
				int pairs = std::atoi(kvPairsNumber);
				//std::cout << "KV-Pairs " << pairs << "\n";
				int key_size, val_size;
				char _key_size[9], _val_size[9];
				std::unique_ptr<char[]> key;
				std::unique_ptr<char[]> value;
				for (int i = 0; i < pairs; i++) {
					::memcpy(_key_size, slice + offset, 8);
					offset += 8;
					::memcpy(_val_size, slice + offset, 8);
					offset += 8;
					_key_size[8] = '\0';
					_val_size[8] = '\0';
					key_size = std::atoi(_key_size);
					val_size = std::atoi(_val_size);

#ifdef DEBUG_EXTRA
					//std::cout << key_size << " " << val_size << "\n";
#endif

					key.reset(new char[key_size+1]);
					value.reset(new char[val_size+1]);

					::memcpy(key.get(), slice + offset, key_size);
					key.get()[key_size] = '\0';
					offset += key_size;

					::memcpy(value.get(), slice + offset, val_size);
					value.get()[val_size] = '\0';
					offset += val_size;

					_map.insert(std::make_pair(std::string(key.get(), key_size), std::string(value.get(), val_size)));
				}
			}
			break;
		case COMMIT:
			char des[9];
			::memcpy(des, slice + 8 + 8 + 8 + 8, 8);
			decision = std::atoi(des);
			break;
		default:
			assert(false);
	}
}


bool CommitLogReader::RecoverLog(int& RID, int& globId, int& tc, int& phase, std::unordered_map<std::string, std::string>& _map) {
	std::cout << __PRETTY_FUNCTION__ << "\n";

	Slice record;
	std::string scratch;
	int decision;
	bool flag = clogReader->ReadRecord(&record, &scratch);
	while (flag) {
		if (flag) {
			decodeRecord(record, RID, globId, tc, phase, decision, _map);
			flag = clogReader->ReadRecord(&record, &scratch);
		}
	}
	return flag;
};
#endif

CommitLogReader::~CommitLogReader() {
#ifdef DEBUG_EXTRA
	std::cout << __PRETTY_FUNCTION__ << "\n";
#endif
};


bool CommitLogReader::RecoverLogEntry(int& RID, int& globId, int& tc, int& phase, std::unordered_map<std::string, std::string>& _map) {
	Slice record;
	std::string scratch;
	int decision;
	bool flag = clogReader->ReadRecord(&record, &scratch);
	if (flag) {
		decodeRecord(record, RID, globId, tc, phase, decision, _map);
	}
	return flag;
};
