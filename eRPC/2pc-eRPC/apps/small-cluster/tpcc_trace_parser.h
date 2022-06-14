#pragma once
#include <iostream>
#include <vector>
#include <thread>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/text_format.h>

#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>

#ifdef SCONE
#include "server_app_scone/message.pb.h"
#else
#include "server_app/message.pb.h"
#endif

#ifndef SCONE
#include <folly/ConcurrentSkipList.h>
#endif

// code for loader
namespace trace_parser {
	constexpr int nb_loader_threads = 8;
	static rocksdb::WriteOptions write_options;

	struct loader_threads_args {
		std::string filename;
		std::string path_of_merged_files;
		rocksdb::TransactionDB* txn_db;
		int tid, cur_node_id, cluster_sz;
		int64_t nb_traces;
	};

	bool should_load(std::string const& key, int const& cur_node_id, int const& cluster_sz) {
		auto id = std::hash<std::string>{}(key)%(cluster_sz);
		return (id == cur_node_id);
	}
#ifndef SCONE
	     typedef folly::ConcurrentSkipList<uint64_t> SkipListT;
#endif

	static void merger(void* args) {
#ifndef SCONE
		std::shared_ptr<SkipListT> prev_keys(SkipListT::createInstance());
		  using Accessor = SkipListT::Accessor;
#else
		std::set<uint64_t> prev_keys;
		// @dimitra add a static_assert here (std::set is not multithreaded)
#endif
		static std::atomic<uint64_t> merged_files_id{0};
		constexpr int merge_factor = 10; // we merge 50 files into 1
		auto th_args = reinterpret_cast<struct loader_threads_args*>(args);

		std::string path = th_args->filename;
		std::string filename = th_args->filename;
		std::string path_of_merged_files = th_args->path_of_merged_files;

		auto tid            = th_args->tid; 
		auto nb_traces      = th_args->nb_traces;

		auto range          = (nb_traces) / nb_loader_threads;
		auto start          = (tid * range);
		auto end            = (tid == 7) ? nb_traces : ((tid + 1) * range);

		uint64_t total_txns = 0;
		int fd = -1;

		auto max_step = merge_factor;
		int i = start;
		while (i < end) {

			tutorial::ClientMessageReq merged_req;
			for (int j = i; j < (i+max_step); j++) {
				total_txns++;
				filename += std::to_string(j);
				fd = open(filename.c_str(), O_RDONLY);
				if (fd < 0) {
					std::cerr << "failed to open the " << filename << "\n";
					exit(-1);
				}

				google::protobuf::io::FileInputStream fileInput(fd);
				fileInput.SetCloseOnDelete(true);
				tutorial::ClientMessageReq client_msg_req;
				if (google::protobuf::TextFormat::Parse(&fileInput, &client_msg_req)) {
					// std::cout << "read input file " << filename << std::endl;
				}
				else {
					int size = -1;
					if ((size = lseek(fd, 0, SEEK_END)) < 0) {
						std::cerr << "error in lseek()\n";
					}


					if (0 == size) {
						// @dimitra: do not break
						std::cerr << "file " << filename << " is empty ..\n";
					}
					else {
						std::cerr << "error in reading input file " << filename << std::endl;
						exit(-1);
					}

				}

				if (i == j) {
					merged_req.set_tostart(true);
					merged_req.set_tocommit(true);
					merged_req.set_clientid(client_msg_req.clientid());
				}

				for (auto k = 0; k < client_msg_req.operations_size(); k++) {
					const tutorial::Statement& s = client_msg_req.operations(k);
					if (s.optype() == tutorial::Statement::READ) {
						continue;
					}
					else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
						continue;
					}
					else if (s.optype() == tutorial::Statement::WRITE) {
						// merged_req add s.key() - s.value()
#ifndef SCONE
						Accessor kv(prev_keys);
						auto [node, found] = kv.addOrGetData(s.key());
						if (found) {
							tutorial::Statement* new_op = merged_req.add_operations();
							new_op->set_key(s.key());
							new_op->set_value(s.value());
							new_op->set_optype(tutorial::Statement::WRITE);
						}
#else
						if (prev_keys.find(s.key()) == prev_keys.end()) {
							tutorial::Statement* new_op = merged_req.add_operations();
							new_op->set_key(s.key());
							new_op->set_value(s.value());
							new_op->set_optype(tutorial::Statement::WRITE);
							prev_keys.insert(s.key());
						}
#endif
					}
				}    
				filename = path;
			}
			// print the protobuf into a file
			/*
			 * static bool TextFormat::Print(*const Message & message, * io::ZeroCopyOutputStream * output)
			 * FileOutputStream::FileOutputStream(int file_descriptor, int block_size = -1)
			 */
			auto id = merged_files_id.fetch_add(1);
			std::string output_filename = path_of_merged_files + "/test_" + std::to_string(id);
			int out_fd = open(output_filename.c_str(), O_WRONLY|O_CREAT, 0777);
			if (out_fd < 0) {
				std::cerr << "failed to open the " << output_filename << "\n";
				std::cerr << std::strerror(errno) << "\n";
				exit(-1);
			}

			google::protobuf::io::FileOutputStream fileOutput(out_fd);
			if (!google::protobuf::TextFormat::Print(merged_req, &fileOutput)) {
				// returns false if ::Print fails
				std::cerr << "failed to write the " << output_filename << "\n";
				std::cerr << std::strerror(errno) << "\n";
				exit(-1);
			}
			fileOutput.SetCloseOnDelete(true);
			auto cur_i = (i + max_step);
			max_step = (end - (i + max_step)) > merge_factor ? merge_factor : (end - (i + max_step));
			if (id % (100*merge_factor) == 0) 
				std::cout << "written files " << merged_files_id.load() << "\n";
			// std::cout << "max_step = " << max_step << " start: " << start << ", end: " << end << ", current i = " << cur_i << "\n";
			i = cur_i;
		}
		fprintf(stdout, "total txns = %" PRIu64 " from %d to %d\n", total_txns, start, end);
	}

	/* 
	 * loader_func() is thread-safe as long as rocksdb::TransactionDB layer for txn
	 * is thread-safe
	 */
	void loader_func(void* args) {
		auto th_args = reinterpret_cast<struct loader_threads_args*>(args);

		std::string path = th_args->filename;
		std::string filename = th_args->filename;


		auto txn_db         = th_args->txn_db;
		auto tid            = th_args->tid; 
		auto cur_node_id    = th_args->cur_node_id;
		auto cluster_sz     = th_args->cluster_sz;
		auto nb_traces      = th_args->nb_traces;

		auto range          = (nb_traces) / nb_loader_threads;
		auto start          = (tid * range);
		auto end            = (tid == 7) ? nb_traces : ((tid + 1) * range);

		uint64_t total_txns = 0;
		rocksdb::Transaction* txn = nullptr;
		rocksdb::Status status;
		int fd = -1;

		for (int i = start; i < end; i++) {
			filename += std::to_string(i);
			fd = open(filename.c_str(), O_RDONLY);
			if (fd < 0) {
				std::cerr << "failed to open the " << filename << "\n";
				exit(-1);
			}

			google::protobuf::io::FileInputStream fileInput(fd);
			fileInput.SetCloseOnDelete(true);
			tutorial::ClientMessageReq client_msg_req;
			if (google::protobuf::TextFormat::Parse(&fileInput, &client_msg_req)) {
				// std::cout << "read input file " << filename << std::endl;
			}
			else {
				std::cerr << "error in reading input file " << filename << std::endl;
				// @dimitra: do not break
			}

			if (txn != nullptr) {
				std::cerr << "txn should be nullptr ..\n";
				exit(-1);
			}

			bool loaded_data = false;
			txn = txn_db->BeginTransaction(write_options);
			for (auto i = 0; i < client_msg_req.operations_size(); i++) {
				const tutorial::Statement& s = client_msg_req.operations(i);
				if (s.optype() == tutorial::Statement::READ) {
					continue;
				}
				else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
					continue;
				}
				else if (s.optype() == tutorial::Statement::WRITE) {
					if (should_load(std::to_string(s.key()), cur_node_id, cluster_sz)) {
						loaded_data = true;
						status = txn->Put(std::to_string(s.key()), s.value());
						if (!status.ok()) {
							fprintf(stderr, "Transaction::Put(%s) %s\n", status.ToString().c_str(), std::to_string(s.key()).c_str());
							// @dimitra we should never reach this point at loading phase -- there are some duplicates
							// exit(-1);
						}
					}
				}
			}    

			if (client_msg_req.tocommit() && loaded_data) {
				status = txn->Commit();

				// @dimitra TODO: potential leak (?)
				delete txn;
				txn = nullptr;
				total_txns += 1;

				if (!status.ok()) {
					fprintf(stderr, "Transaction::Commit %s\n", status.ToString().c_str());
					exit(-1);
				}
			}

			if (!loaded_data) {
				delete txn;
				txn = nullptr;
			}
			filename = path;
		}
		fprintf(stdout, "total txns = %" PRIu64 " from %d to %d\n", total_txns, start, end);
	}


	void multithreaded_loader_workload(std::string const& traces_path, int64_t const& nb_traces, rocksdb::TransactionDB* txn_db, int const& node_id, int const& cluster_size, std::string const& merged_files_path) {

		std::vector<std::thread> loader_threads;
		std::vector<std::unique_ptr<struct loader_threads_args>> loader_args;

		for (int i = 0; i < nb_loader_threads; i++) {
			auto ptr = std::make_unique<struct loader_threads_args>();
			ptr->tid            = i;
			ptr->filename       = traces_path;
			ptr->path_of_merged_files = merged_files_path;
			ptr->txn_db         = txn_db;
			ptr->cur_node_id    = node_id;
			ptr->cluster_sz     = cluster_size;
			ptr->nb_traces      = nb_traces;

			loader_args.emplace_back(std::move(ptr));
			loader_threads.emplace_back(std::thread(loader_func, loader_args.back().get()));
		}


		for (auto& t : loader_threads) {
			t.join();
		}
	}

	void multithreaded_merger(std::string const& traces_path, int64_t const& nb_traces, rocksdb::TransactionDB* txn_db, int const& node_id, int const& cluster_size, std::string const& merged_files_path) {

		std::vector<std::thread> loader_threads;
		std::vector<std::unique_ptr<struct loader_threads_args>> loader_args;

		for (int i = 0; i < nb_loader_threads; i++) {
			auto ptr = std::make_unique<struct loader_threads_args>();
			ptr->tid            = i;
			ptr->filename       = traces_path;
			ptr->path_of_merged_files = merged_files_path;
			ptr->txn_db         = txn_db;
			ptr->cur_node_id    = node_id;
			ptr->cluster_sz     = cluster_size;
			ptr->nb_traces      = nb_traces;

			loader_args.emplace_back(std::move(ptr));
			loader_threads.emplace_back(std::thread(merger, loader_args.back().get()));
		}


		for (auto& t : loader_threads) {
			t.join();
		}
	}



	void loader_workload(std::string const& traces_path, int64_t const& nb_traces, 
			rocksdb::TransactionDB* txn_db, int const& node_id, int const& cluster_size) {
		std::string path = traces_path;
		std::string filename = traces_path;

		uint64_t total_txns = 0;
		rocksdb::Transaction* txn = nullptr;
		rocksdb::Status status;

		for (int i = 0; i < nb_traces; i++) {
			filename += std::to_string(i);
			int fd = open(filename.c_str(), O_RDONLY);
			if (fd < 0) {
				std::cerr << "failed to open the " << filename << "\n";
				exit(-1);
			}
			google::protobuf::io::FileInputStream fileInput(fd);
			fileInput.SetCloseOnDelete(true);

			tutorial::ClientMessageReq client_msg_req;

			if (google::protobuf::TextFormat::Parse(&fileInput, &client_msg_req)) {
				std::cout << "read input file " << filename << std::endl;
			}
			else {
				std::cerr << "error in reading input file " << filename << std::endl;
				// @dimitra: do not break
			}

			if (txn != nullptr) {
				std::cerr << "txn should be nullptr ..\n";
				exit(-1);
			}

			bool loaded_data = false;
			txn = txn_db->BeginTransaction(write_options);
			for (auto i = 0; i < client_msg_req.operations_size(); i++) {
				const tutorial::Statement& s = client_msg_req.operations(i);
				if (s.optype() == tutorial::Statement::READ) {
					continue;
				}
				else if (s.optype() == tutorial::Statement::READ_FOR_UPDATE) {
					continue;
				}
				else if (s.optype() == tutorial::Statement::WRITE) {
					if (should_load(std::to_string(s.key()), node_id, cluster_size)) {
						loaded_data = true;
						status = txn->Put(std::to_string(s.key()), s.value());
						if (!status.ok()) {
							fprintf(stderr, "Transaction::Put(%s) %s\n", status.ToString().c_str(), std::to_string(s.key()).c_str());
							// @dimitra we should never reach this point at loading phase
							exit(-1);
						}
					}
				}
			}

			if (client_msg_req.tocommit() && loaded_data) {
				status = txn->Commit();

				// @dimitra TODO: potential leak (?)
				delete txn;
				txn = nullptr;
				total_txns += 1;

				if (!status.ok()) {
					fprintf(stderr, "Transaction::Commit %s\n", status.ToString().c_str());
					exit(-1);
				}
			}
			if (!loaded_data) {
				delete txn;
				txn = nullptr;
			}
			filename = path;
		}
		fprintf(stdout, "total txns = %" PRIu64 "\n", total_txns);
	}
};

// end code of loader
