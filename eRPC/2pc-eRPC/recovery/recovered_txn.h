#pragma once

class RecoveredOnGoingTxn {
	public:
		using op_type = std::string;
		using key = std::string;
		using value = std::string;
		using _write_batch = std::vector<std::tuple<op_type, key, value>>;

		// RecoveredOnGoingTxn(const RecoveredOnGoingTxn& other) = delete;
		// void operator=(const RecoveredOnGoingTxn& other) = delete;

		// RecoveredOnGoingTxn() = delete;

		RecoveredOnGoingTxn() {};

		RecoveredOnGoingTxn(const RecoveredOnGoingTxn& other) {
			gId = other.gId;
			tc = other.tc;
			phase = other.phase;
			txn_batch = other.txn_batch;
		}

		void operator=(const RecoveredOnGoingTxn& other) {
			gId = other.gId;
			tc = other.tc;
			phase = other.phase;
			txn_batch = other.txn_batch;
		}

		RecoveredOnGoingTxn(int globalId, int Coordinator, int ph, std::unordered_map<std::string, std::string> batch = {}) : gId(globalId), tc(Coordinator), phase(ph) {
			// std::cout << __PRETTY_FUNCTION__ << "\n";
			assert(phase != COMMIT);
			// txn_batch = std::move(batch);
			txn_batch.push_back(std::make_tuple("W", "A", "Lisa Simpson"));
			txn_batch.push_back(std::make_tuple("W", "B", "Ralph Wiggum"));
			// std::cout << __PRETTY_FUNCTION__ <<  " batch size must be 2 == " << txn_batch.size() << "\n";
		};

		~RecoveredOnGoingTxn() {};

		bool emptyBatch() { 
			return (txn_batch.size() == 0);
		}

		RecoveredOnGoingTxn(RecoveredOnGoingTxn&& other) {
			gId = std::move(other.gId);
			tc = std::move(other.tc);
			phase = std::move(other.phase);
			txn_batch = other.txn_batch;
		}

		void operator=(RecoveredOnGoingTxn&& other) {
			gId = std::move(other.gId);
			tc = std::move(other.tc);
			phase = std::move(other.phase);
			txn_batch = std::move(other.txn_batch);
		}

		bool check(int other_tc) {
			return (tc == other_tc);
		}


		int gId, tc, phase;
		_write_batch txn_batch;
	private:
		// txn_handle;

};

