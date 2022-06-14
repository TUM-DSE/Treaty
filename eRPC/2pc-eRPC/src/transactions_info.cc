#include "HashMap.h" // make use of a thread-safe hash-map
#include "transactions_info.h"

#if 0
using id = int; // coordinator's assigned id (no duplicates in same nodes)
using state = int;
using RECEIVED_PREPARE_MESSAGES = int;
using REMAINING_COMMIT_ACKNOWLEDGMENTS = int;


CTSL::HashMap<id, state> _transactions_map;


CTSL::HashMap<id, RECEIVED_PREPARE_MESSAGES> _prep_msg;

CTSL::HashMap<id, REMAINING_COMMIT_ACKNOWLEDGMENTS> _commit_msg;

#endif


// refactoring starts here
CTSL::HashMap<int, struct txn_infos> txn_infos_map;
