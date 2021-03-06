/*******************************************************************************
 * Copyright 2013-2017 Aerospike, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#include "client.h"
#include "async.h"
#include "conversions.h"
#include "policy.h"
#include "log.h"
#include "scan.h"

extern "C" {
	#include <aerospike/aerospike_scan.h>
	#include <aerospike/as_error.h>
	#include <aerospike/as_policy.h>
	#include <aerospike/as_scan.h>
	#include <aerospike/as_status.h>
}

using namespace v8;

NAN_METHOD(AerospikeClient::ScanAsync)
{
	TYPE_CHECK_REQ(info[0], IsString, "namespace must be a string");
	TYPE_CHECK_OPT(info[1], IsString, "set must be a string");
	TYPE_CHECK_OPT(info[2], IsObject, "options must be an object");
	TYPE_CHECK_OPT(info[3], IsObject, "policy must be an object");
	TYPE_CHECK_OPT(info[4], IsNumber, "scan_id must be a number");
	TYPE_CHECK_REQ(info[5], IsFunction, "callback must be a function");

	AerospikeClient* client = ObjectWrap::Unwrap<AerospikeClient>(info.This());
	LogInfo* log = client->log;

	CallbackData* data = new CallbackData();
	data->client = client;
	data->callback.Reset(info[5].As<Function>());

	as_scan scan;
	uint64_t scan_id = 0;
	as_policy_scan policy;
	as_policy_scan* p_policy = NULL;
	as_error err;
	as_status status;

	setup_scan(&scan, info[0], info[1], info[2], log);

	if (info[3]->IsObject()) {
		if (scanpolicy_from_jsobject(&policy, info[3]->ToObject(), log) != AS_NODE_PARAM_OK) {
			as_error_update(&err, AEROSPIKE_ERR_PARAM, "Policy object invalid");
			invoke_error_callback(&err, data);
			goto Cleanup;
		}
		p_policy = &policy;
	}

	if (info[4]->IsNumber()) {
		scan_id = info[4]->ToInteger()->Value();
		as_v8_info(log, "Using scan ID %lli for async scan.", scan_id);
	}

	as_v8_debug(log, "Sending async scan command");
	status = aerospike_scan_async(client->as, &err, p_policy, &scan, &scan_id, async_scan_listener, data, NULL);
	if (status != AEROSPIKE_OK) {
		invoke_error_callback(&err, data);
	}

Cleanup:
	as_scan_destroy(&scan);
}
