// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <string.h>

#include "log.h"
#include "matrix.h"

// Matrix Interface

void ir_matrix_init_lua(lua_State *L) {
	lua_createtable(L, 0, 3);

	lua_pushcfunction(L, ir_matrix_from_lua);
	lua_setfield(L, -2, "from");

	lua_pushcfunction(L, ir_matrix_identity_lua);
	lua_setfield(L, -2, "identity");

	lua_pushcfunction(L, ir_matrix_zero_lua);
	lua_setfield(L, -2, "zero");

	lua_setfield(L, -2, "mat");
}

int ir_matrix_ismatrix(lua_State *L, int index) {
	const char *type;

	if (lua_gettop(L) != 1) {
		lua_pop(L, lua_gettop(L));
		return 0;
	}

	if (!lua_isuserdata(L, index)) {
		lua_pop(L, 1);
		return 0;
	}

	lua_getfield(L, index, "__type");
	if (!lua_isstring(L, -1)) {
		lua_pop(L, 1);
		return 0;
	}

	type = lua_tostring(L, -1);
	if (strcmp(type, "matrix")) {
		lua_pop(L, 1);
		return 0;
	}

	lua_pop(L, 1);
	return 1;
}

void ir_matrix_pushmatrix(lua_State *L, mat4 *victim) {
	mat4 *userdata = lua_newuserdata(L, sizeof(mat4));
	// No hardware accelerated magic here. We can't guarantee that memory
	// allocated by Lua is aligned! ~ahill
	memcpy(userdata, victim, sizeof(mat4));

	lua_createtable(L, 0, 2);

	lua_pushcfunction(L, ir_matrix_index_lua);
	lua_setfield(L, -2, "__index");

	lua_pushcfunction(L, ir_matrix_multiply_lua);
	lua_setfield(L, -2, "__mul");

	lua_setmetatable(L, -2);
}

void ir_matrix_tomatrix(lua_State *L, int index, mat4 *dest) {
	mat4 *userdata;

	if (ir_matrix_ismatrix(L, index)) {
		userdata = lua_touserdata(L, index);
		// No alignment in Lua D: ~ahill
		memcpy(dest, userdata, sizeof(mat4));
	}
}

// Matrix Operations

int ir_matrix_from_lua(lua_State *L) {
	float temp;
	mat4 result;

	if (!lua_istable(L, -1)) {
		return 0;
	}

	if (lua_objlen(L, -1) != 16) {
		return 0;
	}

	for (int i = 1; i <= 16; i++) {
		lua_rawgeti(L, -2, i);
		if (!lua_isnumber(L, -1)) {
			lua_pop(L, 2);
			return 0;
		}
		temp = lua_tonumber(L, -1);
		lua_pop(L, 1);
		result[(i - 1) % 4][(i - 1) / 4] = temp;
	}

	ir_matrix_pushmatrix(L, &result);

	return 1;
}

int ir_matrix_identity_lua(lua_State *L) {
	mat4 victim;
	glm_mat4_identity(victim);
	ir_matrix_pushmatrix(L, &victim);
	return 1;
}

int ir_matrix_index_lua(lua_State *L) {
	const char *key;

	// TODO: Include the ability to directly manipulate the matrix from Lua ~ahill
	if(lua_isstring(L, -1)) {
		key = lua_tostring(L, -1);
		lua_pop(L, 2);
		if(!strcmp(key, "__metatable")) {
			lua_pushstring(L, "My God, what are you doing?");
			return 1;
		} else if(!strcmp(key, "__type")) {
			lua_pushstring(L, "matrix");
			return 1;
		} else if(!strcmp(key, "inverse")) {
			lua_pushcfunction(L, ir_matrix_inverse_lua);
			return 1;
		} else if(!strcmp(key, "transpose")) {
			lua_pushcfunction(L, ir_matrix_transpose_lua);
			return 1;
		}
	}
	return 0;
}

int ir_matrix_inverse_lua(lua_State *L) {
	mat4 input;
	mat4 output;

	if (ir_matrix_ismatrix(L, -1)) {
		ir_matrix_tomatrix(L, -1, &input);
		lua_pop(L, 1);
		glm_mat4_inv(input, output);
		ir_matrix_pushmatrix(L, &output);
		return 1;
	}

	return 0;
}

int ir_matrix_multiply_lua(lua_State *L) {
	// This function makes the assumption that the first argument is always a
	// mat4. If that is not the case, it *will* return nil! ~ahill
	mat4 a;
	mat4 b;
	mat4 result;

	if (lua_gettop(L) != 2) {
		return 0;
	}

	if (!ir_matrix_ismatrix(L, -2)) {
		return 0;
	}

	ir_matrix_tomatrix(L, -2, &a);

	if (ir_matrix_ismatrix(L, -1)) {
		ir_matrix_tomatrix(L, -1, &b);
		lua_pop(L, 2);
		glm_mat4_mul(a, b, result);
		ir_matrix_pushmatrix(L, &result);
		return 1;
	} else {
		return 0;
	}
}

int ir_matrix_transpose_lua(lua_State *L) {
	mat4 input;
	mat4 output;

	if (ir_matrix_ismatrix(L, -1)) {
		ir_matrix_tomatrix(L, -1, &input);
		lua_pop(L, 1);
		glm_mat4_transpose_to(input, output);
		ir_matrix_pushmatrix(L, &output);
		return 1;
	}

	return 0;
}

int ir_matrix_zero_lua(lua_State *L) {
	mat4 userdata;
	glm_mat4_zero(userdata);
	ir_matrix_pushmatrix(L, &userdata);
	return 1;
}

// Vector Interface

void ir_vector_init_lua(lua_State *L) {
	lua_createtable(L, 0, 0);
	// ...
	lua_setfield(L, -2, "vec");
}

// TODO: Vector interface