/***********************************************************************
**
**  REBOL [R3] Language Interpreter and Run-time Environment
**
**  Copyright 2014 Atronix Engineering, Inc.
**  REBOL is a trademark of REBOL Technologies
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**  http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
**
************************************************************************
**
**  Module:  t-library.c
**  Summary: External Library Support
**  Section: datatypes
**  Author:  Shixin Zeng
**  Notes:
**
***********************************************************************/

#include "sys-core.h"

/***********************************************************************
**
*/	REBINT CT_Library(REBVAL *a, REBVAL *b, REBINT mode)
/*
***********************************************************************/
{
	//RL_Print("%s, %d\n", __func__, __LINE__);
	return -1;
}

/***********************************************************************
**
*/	REBTYPE(Library)
/*
***********************************************************************/
{
	REBVAL *val;
	REBVAL *arg;
	REBSTU *strut;
	REBSTU *nstrut;
	REBCNT index;
	REBCNT tail;
	REBCNT len;

	arg = D_ARG(2);
	val = D_ARG(1);
	strut = 0;

	REBVAL *ret = DS_RETURN;
	// unary actions
	switch(action) {
		case A_MAKE:
			//RL_Print("%s, %d, Make library action\n", __func__, __LINE__);
		case A_TO:
			if (!IS_DATATYPE(val)) {
				Trap_Types(RE_EXPECT_VAL, REB_LIBRARY, VAL_TYPE(val));
			}
			if (!IS_FILE(arg)) {
				Trap_Types(RE_EXPECT_VAL, REB_BLOCK, VAL_TYPE(arg));
			} else {
				REBCNT len = VAL_LEN(arg);
				REBYTE *path = (REBYTE*)OS_MAKE(len + 1);
				void *lib = NULL;
				memcpy(path, VAL_DATA(arg), len);
				path[len] = '\0';
				lib = OS_OPEN_LIBRARY(path, NULL);
				if (!lib) {
					OS_Free(path);
					Trap_Make(REB_LIBRARY, arg);
				}
				OS_Free(path);
				VAL_LIB_HANDLE(ret) = (REBLHL*)Make_Node(LIB_POOL);
				VAL_LIB_FD(ret) = lib;
				USE_LIB(VAL_LIB_HANDLE(ret));
				SET_TYPE(ret, REB_LIBRARY);
			}
			break;
		default:
			Trap_Action(REB_LIBRARY, action);
	}
	return R_RET;
}
