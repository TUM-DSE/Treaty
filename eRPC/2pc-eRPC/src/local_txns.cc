#include "local_txns.h"
#include <memory>

/**
 * A map of the ongoing sub-transactions in this node.
 * The CTSL::HashMap is a thread-safe hash-map that can
 * be safely accessed concurrently by many threads.
 */
CTSL::HashMap<std::string, std::unique_ptr<LocalTxn>>* local_txns;


/**
 * It returns true is the transaction with the 
 * given id (argument) exists in the local_txns map.
 * Otherwise, it returns false.
 */
bool transaction_exists(const char* txnId) {
	if (local_txns->find(std::string(txnId)))
		return true;
	return false;
}


/**
 * Returns a handle (pointer) to the sub-tranasction object
 * specified by the id (argument).
 */
LocalTxn* get_transaction(std::string id) {
	if (local_txns->find(id))
		return (*local_txns)[id].get();
	return nullptr;
}

// debugging
void print_local_txns() {
	LOG_DEBUG_HANDLERS(__FILE__, __LINE__, __PRETTY_FUNCTION__);
	//FIXME: needs to be implemented
}


std::shared_ptr<PacketSsl> txn_cipher;

std::array<int, 4096> global_fiber_id;
