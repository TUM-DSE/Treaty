/**
 * Requests' Handlers that are invoked when the thread that is registered to a
 * single rpc object received a request from the remote (matched) end-point.
 * These handlers are executed by participants; the coordinators will execute
 * the continuation functions that are associated with each specific request.
 */

#pragma once

#include "rpc.h"

/**
 * Transactions' related Handlers; will be invoked from participants threads 
 * when a sub transaction needs to be processed.
 */
void req_handler_txnCommit(erpc::ReqHandle *req_handle, void *context);

void req_handler_txnBegin(erpc::ReqHandle *req_handle, void *context);

void req_handler_txnPut(erpc::ReqHandle *req_handle, void *context);
void req_handler_txnDelete(erpc::ReqHandle *req_handle, void *context);

void req_handler_txnRead(erpc::ReqHandle *req_handle, void *context);
void req_handler_txnReadForUpdate(erpc::ReqHandle *req_handle, void *context);

void req_handler_txnPrepare(erpc::ReqHandle *req_handle, void *context);
void req_handler_txnRollback(erpc::ReqHandle *req_handle, void *context);

/**
 * This request-handler will trigger the ctrl_handler (exit(1)) and will "gracefully"
 * terminate the execution.
 */
void req_handler_terminate(erpc::ReqHandle *req_handle, void *context);

/**
 * Recovery-Handlers.
 */
void req_handler_recoverParticipant(erpc::ReqHandle *req_handle, void* context);

void req_handler_recoverCoordinator(erpc::ReqHandle *req_handle, void* context);

void create_local_txn(char*);

// Invoked when a session is created, etc.
void sm_handler(int, erpc::SmEventType, erpc::SmErrType, void *);
