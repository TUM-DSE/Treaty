/**
 * Per-Node thread-safe hash-maps that are used
 * for bookeeping purposes.
 */

#pragma once 
#include "HashMap.h" // make use of a thread-safe hash-map

/**
 * Represents the state of a local sub-transaction.
 */
enum LocalTxnState {
        kInProgress = 0,
        kPrepared,
        kCommitted,
        kAborted,
	kToBeAborted
};


#if 0
/**
 * _transactions_map holds globally for each distributed
 * transaction its state.
 * The pair is <TxnId, State>, where the id is the one
 * assigned by the coordinator that created this specific
 * transaction.
 * Hint: Needed for the participants-recovery .
 */
extern CTSL::HashMap<int, int> _transactions_map;


/**
 * _prep_msg holds for each txn (TxnId = the coordinator's
 * assigned global id) the number of the received
 * prepared messages.
 * This is necessary for a transaction; all sub-transactions
 * need to be successfully prepared in order to commit().
 */
extern CTSL::HashMap<int, int> _prep_msg;


extern CTSL::HashMap<int, int> _commit_msg;


extern CTSL::HashMap<int, int> _commit_msg;
#endif

/* this starts the refactored */
struct txn_infos {
        int finished_ops;
        int recv_prep_msg;
        int commit_acks;
        int txn_state;
};

extern CTSL::HashMap<int, struct txn_infos> txn_infos_map;
