/* Copyright 2022 Peter Wagener <mail@peterwagener.net>

This file is part of Freyr2.

Freyr2 is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

Freyr2 is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
Freyr2. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MODULE_API_H
#define MODULE_API_H

#ifdef __cplusplus
extern "C" {
#endif
#include "unicorn/idl.h"
#include "frame_api.h"
#include "types/stringlist.h"

typedef size_t modno_t;
#define INVALID_MODULE ((modno_t)(0))

#define EXPORT [[gnu::visibility("default")]]

typedef unsigned module_sync_token_t;

typedef enum response_type_t { E, W, I, D, T } response_type_t;

typedef void (*module_init_f)(
	modno_t modno, const char *argstr, void **puserdata);
typedef void (*module_deinit_f)(modno_t modno, void *userdata);
typedef void (*module_flush_f)(modno_t modno, void *userdata);

typedef void (*command_response_f)(
	response_type_t type,
	const char *    source,
	const char *    response,
	void *          userdata);

typedef void (*command_func_f)(
	modno_t modno, const char *argstr, void *userdata);
typedef uidl_node_t *(*command_describe_f)(void *userdata);

typedef size_t hook_t;
#define INVALID_HOOK ((hook_t)(-1))
typedef void (*hook_func_f)(hook_t hook, modno_t modno, void *userdata);

void module_status();

stringlist_t *module_list_get();

modno_t module_instantiate(
	const char *ident, const char *instanceName, const char *argstring);
modno_t module_find(const char *instanceName);
void    module_remove(modno_t modno);
void    module_cleanup();

void module_register_command(
	modno_t            modno,
	const char *       ident,
	command_func_f     func,
	command_describe_f describe);

int  command_run(const char *cmdline, const char *source);
void command_respond_default(
	response_type_t type, const char *source, const char *response, void *);

void command_response_push(command_response_f re_func, void *re_userdata);
void command_response_pop();
void command_respond(
	response_type_t type, const char *source, const char *response);

uidl_node_t *commands_describe();

hook_t hook_resolve(const char *ident);
void   module_hook(modno_t modno, hook_t hook, hook_func_f func);
void   hook_trigger(hook_t hook);

void main_start();
void main_stop();
int  main_running();

#ifdef __cplusplus
}
#endif

#endif