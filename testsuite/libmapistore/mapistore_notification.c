/*
   OpenChange Unit Testing

   OpenChange Project

   Copyright (C) Julien Kerihuel 2015

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "testsuite.h"
#include "mapiproxy/libmapistore/mapistore.h"
#include "mapiproxy/libmapistore/mapistore_private.h"
#include "mapiproxy/libmapistore/mapistore_errors.h"

/* Global variables */
static struct GUID	gl_async_uuid;
static struct GUID	gl_uuid;
static const char	*gl_cn = "FooBar";
static const char	*gl_host1 = "tcp://host1:9005";
static const char	*gl_host2 = "tcp://host2:9006";
static const char	*gl_host3 = "tcp://host3:9007";

START_TEST(test_initialization) {
	TALLOC_CTX				*mem_ctx = NULL;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	bool					bret;

	mem_ctx = talloc_named(NULL, 0, "test_initialization");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	/* check sanity check compliance */
	retval = mapistore_notification_init(mem_ctx, NULL, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, NULL);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	/* check fallback behavior */
	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* check invalid memcached server configuration settings */
	bret = lpcfg_set_cmdline(lp_ctx, "mapistore:notification_cache", "--SERVER=");
	ck_assert(bret == true);
	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_ERR_CONTEXT_FAILED);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST

START_TEST(session_add) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;
	const char				*cn = NULL;

	/* Check sanity check compliance */
	retval = mapistore_notification_session_add(NULL, gl_uuid, gl_async_uuid, cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_session_add(&mstore_ctx, gl_uuid, gl_async_uuid, cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_session_add(&mstore_ctx, gl_uuid, gl_async_uuid, cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "session_add");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* check when cn is NULL: ndr_push_error expected */
	mstore_ctx.notification_ctx = ctx;
	retval = mapistore_notification_session_add(&mstore_ctx, gl_uuid, gl_async_uuid, cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_DATA);

	/* register session */
	cn = gl_cn;
	retval = mapistore_notification_session_add(&mstore_ctx, gl_uuid, gl_async_uuid, cn);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* check if session is registered */
	retval = mapistore_notification_session_exist(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* attempt to double register the session */
	retval = mapistore_notification_session_add(&mstore_ctx, gl_uuid, gl_async_uuid, cn);
	ck_assert_int_eq(retval, MAPISTORE_ERROR);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST


START_TEST(session_exist) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;

	/* Check sanity check compliance */
	retval = mapistore_notification_session_exist(NULL, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_session_exist(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_session_exist(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "session_exist");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	mstore_ctx.notification_ctx = ctx;

	/* Test non-existent key */
	retval = mapistore_notification_session_exist(&mstore_ctx, GUID_random());
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	/* Test global async_uuid key */
	retval = mapistore_notification_session_exist(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST


START_TEST(session_get) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;
	struct GUID				uuid;
	char					*cnp = NULL;

	/* Check sanity check compliance */
	retval = mapistore_notification_session_get(NULL, NULL, gl_async_uuid, &uuid, &cnp);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	retval = mapistore_notification_session_get(NULL, &mstore_ctx, gl_async_uuid, NULL, &cnp);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	retval = mapistore_notification_session_get(NULL, &mstore_ctx, gl_async_uuid, &uuid, NULL);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_session_get(NULL, &mstore_ctx, gl_async_uuid, &uuid, &cnp);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_session_get(NULL, &mstore_ctx, gl_async_uuid, &uuid, &cnp);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "session_get");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	mstore_ctx.notification_ctx = ctx;

	/* Retrieve non existent record */
	retval = mapistore_notification_session_get(mem_ctx, &mstore_ctx, GUID_random(), &uuid, &cnp);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	/* Retrieve global async_uuid record data */
	retval = mapistore_notification_session_get(mem_ctx, &mstore_ctx, gl_async_uuid, &uuid, &cnp);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	ck_assert_str_eq(gl_cn, cnp);
	ck_assert(GUID_compare(&gl_async_uuid, &uuid));

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST


START_TEST(session_delete) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;

	/* Check sanity check compliance */
	retval = mapistore_notification_session_delete(NULL, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_session_delete(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_session_delete(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "session_delete");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	mstore_ctx.notification_ctx = ctx;

	/* Try to delete non existing key */
	retval = mapistore_notification_session_delete(&mstore_ctx, GUID_random());
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	/* Try to delete global session */
	retval = mapistore_notification_session_delete(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* try to delete twice */
	retval = mapistore_notification_session_delete(&mstore_ctx, gl_async_uuid);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST


START_TEST(resolver_add) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;

	/* Check sanity check compliance */
	retval = mapistore_notification_resolver_add(NULL, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	retval = mapistore_notification_resolver_add(&mstore_ctx, NULL, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, NULL);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "resolver_add");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* check when cn has space */
	mstore_ctx.notification_ctx = ctx;
	retval = mapistore_notification_resolver_add(&mstore_ctx, "Foo Bar", gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_DATA);

	/* add record */
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* check if record now exists */
	retval = mapistore_notification_resolver_exist(&mstore_ctx, gl_cn);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* try to register gl_host1 twice */
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_EXIST);

	/* add 2nd host to gl_cn record */
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host2);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* add 3rd host to gl_cn record */
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host3);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* try to add 2nd host again to gl_cn record */
	retval = mapistore_notification_resolver_add(&mstore_ctx, gl_cn, gl_host2);
	ck_assert_int_eq(retval, MAPISTORE_ERR_EXIST);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST

START_TEST(resolver_exist) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;

	/* Check sanity check compliance */
	retval = mapistore_notification_resolver_exist(NULL, gl_cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	retval = mapistore_notification_resolver_exist(&mstore_ctx, NULL);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_resolver_exist(&mstore_ctx, gl_cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_resolver_exist(&mstore_ctx, gl_cn);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "resolver_exist");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	mstore_ctx.notification_ctx = ctx;

	/* Test gl_cn existence */
	retval = mapistore_notification_resolver_exist(&mstore_ctx, gl_cn);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* Test non existent key */
	retval = mapistore_notification_resolver_exist(&mstore_ctx, "nonexistent");
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST

START_TEST(resolver_get) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;
	uint32_t				count = 0;
	const char				**hosts;

	/* Check sanity check compliance */
	retval = mapistore_notification_resolver_get(NULL, NULL, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	retval = mapistore_notification_resolver_get(NULL, &mstore_ctx, NULL, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	retval = mapistore_notification_resolver_get(NULL, &mstore_ctx, gl_cn, NULL, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	retval = mapistore_notification_resolver_get(NULL, &mstore_ctx, gl_cn, &count, NULL);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_resolver_get(NULL, &mstore_ctx, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_resolver_get(NULL, &mstore_ctx, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "resolver_exist");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	mstore_ctx.notification_ctx = ctx;

	/* Retrieve non existent record */
	retval = mapistore_notification_resolver_get(mem_ctx, &mstore_ctx, "nonexistent", &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	/* Retrieve gl_cn record data */
	retval = mapistore_notification_resolver_get(mem_ctx, &mstore_ctx, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	ck_assert_int_eq(count, 3);
	ck_assert_str_eq(hosts[0], gl_host1);
	ck_assert_str_eq(hosts[1], gl_host2);
	ck_assert_str_eq(hosts[2], gl_host3);
	talloc_free(hosts);

	talloc_free(lp_ctx);
	talloc_free(mem_ctx);

} END_TEST

START_TEST(resolver_delete) {
	TALLOC_CTX				*mem_ctx;
	struct mapistore_context		mstore_ctx;
	enum mapistore_error			retval;
	struct loadparm_context			*lp_ctx;
	struct mapistore_notification_context	*ctx = NULL;
	struct mapistore_notification_context	_ctx;
	uint32_t				count = 0;
	const char				**hosts = NULL;

	/* Check sanity check compliance */
	retval = mapistore_notification_resolver_delete(NULL, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_INITIALIZED);

	retval = mapistore_notification_resolver_delete(&mstore_ctx, NULL, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, NULL);
	ck_assert_int_eq(retval, MAPISTORE_ERR_INVALID_PARAMETER);

	mstore_ctx.notification_ctx = NULL;
	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	mstore_ctx.notification_ctx = &_ctx;
	mstore_ctx.notification_ctx->memc_ctx = NULL;
	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_AVAILABLE);

	/* Initialize mapistore notification system */
	mem_ctx = talloc_named(NULL, 0, "session_delete");
	ck_assert(mem_ctx != NULL);

	lp_ctx = loadparm_init(mem_ctx);
	ck_assert(lp_ctx != NULL);

	retval = mapistore_notification_init(mem_ctx, lp_ctx, &ctx);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	mstore_ctx.notification_ctx = ctx;

	/* Try to delete non existent key */
	retval = mapistore_notification_resolver_delete(&mstore_ctx, "nonexistent", gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	/* Try to delete gl_host1 from gl_cn */
	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	/* Try to delete gl_host1 twice */
	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, gl_host1);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

	/* Check that record now only have gl_host2 and gl_host3 */
	retval = mapistore_notification_resolver_get(mem_ctx, &mstore_ctx, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	ck_assert_int_eq(count, 2);
	ck_assert_str_eq(hosts[0], gl_host2);
	ck_assert_str_eq(hosts[1], gl_host3);
	talloc_free(hosts);

	/* Try to delete gl_host2 */
	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, gl_host2);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	retval = mapistore_notification_resolver_get(mem_ctx, &mstore_ctx, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);
	ck_assert_int_eq(count, 1);
	ck_assert_str_eq(hosts[0], gl_host3);
	talloc_free(hosts);

	/* Try to delete gl_host3 */
	retval = mapistore_notification_resolver_delete(&mstore_ctx, gl_cn, gl_host3);
	ck_assert_int_eq(retval, MAPISTORE_SUCCESS);

	retval = mapistore_notification_resolver_get(mem_ctx, &mstore_ctx, gl_cn, &count, &hosts);
	ck_assert_int_eq(retval, MAPISTORE_ERR_NOT_FOUND);

} END_TEST

Suite *mapistore_notification_suite(void)
{
	Suite	*s;
	TCase	*tc_config;
	TCase	*tc_session;
	TCase	*tc_resolver;

	s = suite_create("libmapistore notification");

	gl_async_uuid = GUID_random();
	gl_uuid = GUID_random();

	/* Configuration */
	tc_config = tcase_create("initialization");
	tcase_add_test(tc_config, test_initialization);
	suite_add_tcase(s, tc_config);

	/* Session management */
	tc_session = tcase_create("session management");
	tcase_add_test(tc_session, session_add);
	tcase_add_test(tc_session, session_exist);
	tcase_add_test(tc_session, session_get);
	tcase_add_test(tc_session, session_delete);
	suite_add_tcase(s, tc_session);

	/* Resolver */
	tc_resolver = tcase_create("resolver API");
	tcase_add_test(tc_resolver, resolver_add);
	tcase_add_test(tc_resolver, resolver_exist);
	tcase_add_test(tc_resolver, resolver_get);
	tcase_add_test(tc_resolver, resolver_delete);
	suite_add_tcase(s, tc_resolver);

	return s;
}
